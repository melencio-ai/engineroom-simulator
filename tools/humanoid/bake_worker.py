"""Quaternius Worker GLB -> skinned Blender poses -> PSP EWHM animation sidecar.

Run with:
 blender -b -noaudio --python tools/humanoid/bake_worker.py -- \
   --input /tmp/Worker.glb --output dist/humanoid/ENGINEER.EWM \
   --manifest dist/humanoid/manifest.json --triangles 560

The output is a strict engine-specific triangle-list format, not a standalone
asset pack and not a glTF/FBX runtime loader. Blender bakes the *real source*
skinning/animations; PSP plays a tiny set of CPU-interpolated pose frames.
"""
import argparse
import json
import math
from pathlib import Path
import struct
import sys

import bpy
from mathutils import Vector


def parameters():
    argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    p = argparse.ArgumentParser()
    p.add_argument("--input", required=True)
    p.add_argument("--output", required=True, type=Path)
    p.add_argument("--manifest", required=True, type=Path)
    p.add_argument("--triangles", type=int, default=560)
    return p.parse_args(argv)


def tri_count(objects):
    return sum(sum(len(p.vertices) - 2 for p in obj.data.polygons)
               for obj in objects)


def apply_modifier(obj, mod):
    bpy.ops.object.select_all(action="DESELECT")
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    # Put topology simplification before the armature modifier, otherwise
    # applying it would bake a single T-pose instead of retaining skinning.
    while obj.modifiers.find(mod.name) > 0:
        bpy.ops.object.modifier_move_up(modifier=mod.name)
    bpy.ops.object.modifier_apply(modifier=mod.name)
    obj.select_set(False)


def reduce_model(meshes, budget):
    for obj in meshes:
        mod = obj.modifiers.new("PSP_triangle_setup", "TRIANGULATE")
        apply_modifier(obj, mod)
    original = tri_count(meshes)
    if original < 100:
        raise ValueError("source is too sparse to be a recognizable humanoid")
    if original > budget:
        ratio = min(0.95, budget / float(original) * 0.97)
        # Decimate all original mesh parts proportionally, without dropping
        # any outfit slot. Preserve vertex groups needed for the armature.
        for obj in meshes:
            if len(obj.data.polygons) <= 10:
                continue
            mod = obj.modifiers.new("PSP_low_poly", "DECIMATE")
            mod.decimate_type = "COLLAPSE"
            mod.ratio = max(0.015, ratio)
            apply_modifier(obj, mod)
        # Required after Blender's decimator can produce quads.
        for obj in meshes:
            mod = obj.modifiers.new("PSP_triangle_final", "TRIANGULATE")
            apply_modifier(obj, mod)
    current = tri_count(meshes)
    if current > budget:
        # A handful of protected tiny accessory meshes can keep the total
        # over target. Decimate the largest parts in a second deterministic
        # pass; do not silently discard the head or hands.
        for _ in range(8):
            if current <= budget:
                break
            candidate = max(meshes, key=lambda m: len(m.data.polygons))
            mod = candidate.modifiers.new("PSP_final_budget", "DECIMATE")
            mod.ratio = max(0.10, (budget / current) * 0.90)
            apply_modifier(candidate, mod)
            mod = candidate.modifiers.new("PSP_final_tris", "TRIANGULATE")
            apply_modifier(candidate, mod)
            current = tri_count(meshes)
    if current > budget or current < 170:
        raise ValueError(f"PSP conversion out of geometry budget: {current} tris")
    return original, current


def find_action(name):
    actions = list(bpy.data.actions)
    exact = [a for a in actions if a.name.lower() == name.lower()]
    if exact:
        return exact[0]
    suffix = [a for a in actions if a.name.lower().endswith(name.lower())]
    if suffix:
        return suffix[0]
    contains = [a for a in actions if name.lower() in a.name.lower()]
    if contains:
        return contains[0]
    raise ValueError(f"Required {name!r} clip missing; found "
                     + ", ".join(a.name for a in actions))


def rgba_to_abgr(r, g, b):
    r = int(max(0, min(255, round(r * 255))))
    g = int(max(0, min(255, round(g * 255))))
    b = int(max(0, min(255, round(b * 255))))
    return 0xff000000 | b << 16 | g << 8 | r


def base_color(material, mesh, polygon, triangle):
    if material is None:
        return 0xff8899aau
    factor = list(material.diffuse_color)[:3]
    image = None
    if material.use_nodes:
        principled = next((n for n in material.node_tree.nodes
                          if n.type == "BSDF_PRINCIPLED"), None)
        if principled:
            color_input = principled.inputs.get("Base Color")
            if color_input:
                factor = list(color_input.default_value)[:3]
                if color_input.is_linked:
                    origin = color_input.links[0].from_node
                    if origin.type == "TEX_IMAGE":
                        image = origin.image
    if image and mesh.uv_layers.active and image.has_data:
        uv_layer = mesh.uv_layers.active.data
        count = len(triangle.loops)
        u = sum(uv_layer[k].uv.x for k in triangle.loops) / count
        v = sum(uv_layer[k].uv.y for k in triangle.loops) / count
        width, height = image.size
        if width > 0 and height > 0:
            px = int((u % 1.0) * (width - 1))
            py = int((v % 1.0) * (height - 1))
            channels = image.channels
            idx = (py * width + px) * channels
            pix = image.pixels
            if len(pix) > idx + 2:
                factor = [factor[j] * pix[idx + j] for j in range(3)]
    return rgba_to_abgr(*factor)


def sample_scene(scene, rigs, meshes, action, progress, palette=None):
    for rig in rigs:
        rig.animation_data_create()
        rig.animation_data.action = action
    start, end = action.frame_range
    frame = start + (end - start) * progress
    scene.frame_set(int(frame), subframe=frame - int(frame))
    dg = bpy.context.evaluated_depsgraph_get()
    dg.update()
    vertices = []
    color_out = []
    for obj in meshes:
        evaluated = obj.evaluated_get(dg)
        evaluated_mesh = evaluated.to_mesh(
            preserve_all_data_layers=True, depsgraph=dg)
        if evaluated_mesh is None:
            raise ValueError(f"Cannot evaluate skinned mesh {obj.name}")
        evaluated_mesh.calc_loop_triangles()
        mats = [s.material for s in obj.material_slots]
        for tri in evaluated_mesh.loop_triangles:
            polygon = evaluated_mesh.polygons[tri.polygon_index]
            material = (mats[polygon.material_index]
                        if polygon.material_index < len(mats) else None)
            abgr = (base_color(material, evaluated_mesh, polygon, tri)
                    if palette is None else palette[len(color_out)])
            for vertex_index in tri.vertices:
                p = evaluated.matrix_world @ evaluated_mesh.vertices[vertex_index].co
                # glTF forward (-Y after Blender import) -> game forward +X;
                # Blender vertical Z -> game vertical Y; Blender X -> game Z.
                vertices.append((-p.y, p.z, p.x))
                color_out.append(abgr)
        evaluated.to_mesh_clear()
    return vertices, color_out


def bake(args):
    bpy.ops.object.select_all(action="SELECT")
    bpy.ops.object.delete(use_global=False)
    bpy.ops.import_scene.gltf(filepath=str(Path(args.input).resolve()))
    meshes = sorted([o for o in bpy.context.scene.objects
                     if o.type == "MESH" and o.visible_get()],
                    key=lambda m: m.name)
    rigs = sorted([o for o in bpy.context.scene.objects if o.type == "ARMATURE"],
                  key=lambda m: m.name)
    if not meshes or not rigs:
        raise ValueError("Expected Quaternius Worker with real skeleton and meshes")
    # Source clips should control the rig, not imported NLA strip overrides.
    for rig in rigs:
        rig.animation_data_create()
        for track in rig.animation_data.nla_tracks:
            track.mute = True
    original, final_tris = reduce_model(meshes, args.triangles)
    clips = [("idle", 6), ("walk", 12), ("interact", 8)]
    poses = []
    colors = None
    for name, frames in clips:
        action = find_action(name)
        for i in range(frames):
            verts, new_colors = sample_scene(
                bpy.context.scene, rigs, meshes, action,
                i / float(frames), colors)
            if colors is None:
                colors = new_colors
            if len(verts) != final_tris * 3:
                raise ValueError("Animated topology changed between samples")
            poses.append(verts)
    if not poses:
        raise ValueError("No actual source rig animations evaluated")

    # Keep root motion in GAME controller only. The source animation is
    # centered per-frame, leaving natural joint motion but no double travel.
    first = poses[0]
    floor = min(p[1] for p in first)
    ceiling = max(p[1] for p in first)
    height = ceiling - floor
    if not 0.3 < height < 30.0:
        raise ValueError(f"Unexpected raw character height {height}")
    scale = 1.82 / height
    frame_size = len(first)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    with args.output.open("wb") as fp:
        fp.write(struct.pack("<4sIIIIIff", b"EWHM", 1, frame_size,
                             clips[0][1], clips[1][1], clips[2][1],
                             1.82, 12.0))
        for pose in poses:
            avg_x = (min(p[0] for p in pose) + max(p[0] for p in pose)) / 2
            avg_z = (min(p[2] for p in pose) + max(p[2] for p in pose)) / 2
            for index, pos in enumerate(pose):
                x = (pos[0] - avg_x) * scale
                y = (pos[1] - floor) * scale
                z = (pos[2] - avg_z) * scale
                if not all(math.isfinite(q) for q in (x, y, z)):
                    raise ValueError("Non-finite Blender skinned vertex")
                fp.write(struct.pack("<Ifff", colors[index], x, y, z))
    if args.output.stat().st_size > 1350000:
        raise ValueError("Imported Worker pose bank exceeds 1.35MB PSP budget")
    profile = {
        "importer": "Engine Watch G3.7 Blender rigid triangle-stream animation",
        "original_creator": "Quaternius",
        "source": "Ultimate Modular Men Pack (2022) / Male Worker",
        "license_source": "https://quaternius.com/packs/ultimatemodularcharacters.html",
        "source_license": "CC0-1.0 (shown on individual pack page)",
        "source_master_is_not_stored_in_repository": True,
        "original_triangles": original,
        "psp_triangles": final_tris,
        "psp_vertices_per_frame": frame_size,
        "frames": {name: n for name, n in clips},
        "total_frames": len(poses),
        "height_m": 1.82,
        "animation_source": [find_action(c).name for c, _ in clips],
        "rig_count": len(rigs),
        "part_count": len(meshes),
        "ewmh_file_bytes": args.output.stat().st_size,
        "limitations": [
            "PSP uses baked source-skeleton vertex frames, not GPU skinning",
            "Ladder uses Walk clip until dedicated source climbing retarget",
            "G3.6 physically targeted arm IK temporarily uses procedural rig",
            "No detailed UV texture on PSP; baked flat color per triangle",
        ],
    }
    args.manifest.parent.mkdir(parents=True, exist_ok=True)
    args.manifest.write_text(json.dumps(profile, indent=2) + "\n")
    print("EWHM PASS:", json.dumps(profile, indent=2))


if __name__ == "__main__":
    bake(parameters())
