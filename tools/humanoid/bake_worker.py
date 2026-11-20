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
    p.add_argument("--triangles", type=int, default=900)
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


def mesh_priority(obj, face_count):
    """Small outfit/face/hand/boot pieces need more topology per face.

    This is weighted per real imported mesh, not a blanket 10% collapse
    across every item. A character with only a few original submeshes still
    benefits from the larger 3P-only PSP character draw budget.
    """
    name = obj.name.lower()
    critical = ("head", "face", "hair", "helmet", "hat", "hand",
                "glove", "shoe", "boot", "eye")
    if any(word in name for word in critical):
        return 3.0
    if face_count < 400:
        return 2.3
    if face_count < 950:
        return 1.7
    return 1.0


def reduce_model(meshes, budget):
    """Per-mesh silhouette-aware LOD: retain small anatomical/outfit parts.

    The old 542-triangle conversion collapsed ALL 5,240 source triangles
    to ~10%, turning the hands/head into visually broken fragments.
    This version budgets a separate 3,000-vertex PSP character draw; the
    ship mesh remains unchanged and has its own independent GE limit.
    """
    for obj in meshes:
        mod = obj.modifiers.new("PSP_triangle_setup", "TRIANGULATE")
        apply_modifier(obj, mod)
    originals = {o.name: len(o.data.polygons) for o in meshes}
    original = sum(originals.values())
    if original < 100 or budget > 960 or budget < 550:
        raise ValueError("invalid source size or PSP LOD target")

    # Establish a minimum for small-but-important individual objects,
    # then fill the remainder by weighted object size and significance.
    priorities = {o.name: mesh_priority(o, originals[o.name]) for o in meshes}
    # The pinned Worker is four specific skinned meshes. A previous
    # greedy allocator spent 441 of 875 triangles ON THE HEAD and left
    # only 95 for two legs, explaining the fragmented suit/limbs in the
    # user's PPSSPP footage. Reserve each anatomically important part.
    target_share = {
        "Worker_Body": .50, "Worker_Legs": .20,
        "Worker_Head": .22, "Worker_Feet": .08
    }
    if set(originals)==set(target_share):
        allocation = {
            name:min(originals[name],round(budget*target_share[name]))
            for name in originals
        }
        # Integer rounding and part-capping can leave unassigned triangles.
        for _ in range(budget - sum(allocation.values())):
            key=max((name for name in originals
                     if allocation[name]<originals[name]),
                    key=lambda name:originals[name]/max(allocation[name],1))
            allocation[key]+=1
    else:
        # A future verified mesh layout uses a conservative generic
        # proportional allocator rather than assuming four Worker parts.
        total_weight=sum(
            originals[name]**.72 * priorities[name]
            for name in originals
        )
        allocation={}
        for name in originals:
            ratio=(originals[name]**.72*priorities[name])/total_weight
            allocation[name]=min(originals[name],max(24,int(budget*ratio)))
        while sum(allocation.values())<budget:
            names=[name for name in originals
                   if allocation[name]<originals[name]]
            if not names:
                break
            key=max(names,key=lambda name:originals[name]/allocation[name])
            allocation[key]+=1
        if sum(allocation.values())>budget:
            raise ValueError("Too many critical anatomy parts for the PSP LOD")

    for obj in meshes:
        count = originals[obj.name]
        keep = allocation[obj.name]
        if count <= keep or count < 20:
            continue
        mod = obj.modifiers.new("PSP_anatomy_preserving_LOD", "DECIMATE")
        mod.decimate_type = "COLLAPSE"
        mod.ratio = min(.999, max(.02, keep / float(count) * .975))
        apply_modifier(obj, mod)
        mod = obj.modifiers.new("PSP_triangle_final", "TRIANGULATE")
        apply_modifier(obj, mod)
    current = tri_count(meshes)
    # Give a few stubborn meshes a final lightweight correction, but never
    # erase the last facial, helmet or safety-footwear geometry.
    for _ in range(10):
        if current <= budget:
            break
        candidates = [o for o in meshes
                      if len(o.data.polygons) >
                      min(originals[o.name], max(24, allocation[o.name] // 2))]
        if not candidates:
            break
        obj = max(candidates, key=lambda o: len(o.data.polygons) /
                  max(1, allocation[o.name]))
        count = len(obj.data.polygons)
        mod = obj.modifiers.new("PSP_late_rebalance", "DECIMATE")
        mod.ratio = max(.40, (budget / current) * .95)
        apply_modifier(obj, mod)
        mod = obj.modifiers.new("PSP_late_triangles", "TRIANGULATE")
        apply_modifier(obj, mod)
        current = tri_count(meshes)
        if len(obj.data.polygons) >= count and current > budget:
            break
    if current > budget or current < 550:
        raise ValueError(f"Anatomical PSP LOD outside budget: {current} tris")
    report = [{
        "name": obj.name,
        "source_triangles": originals[obj.name],
        "target_triangles": allocation[obj.name],
        "result_triangles": len(obj.data.polygons),
        "preservation_weight": priorities[obj.name],
    } for obj in meshes]
    print("ANATOMY_AWARE_LOD:", json.dumps(report))
    return original, current, report


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
        return 0xff8899aa
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


# A coherent, non-UV-dependent shipboard workwear palette. The old
# atlas-centroid sampling made heavily decimated triangles alternate between
# unrelated dark and white texture islands, which looked like torn limbs.
# Use real imported mesh/skin-bone semantics to paint a recognizable outfit.
MARINE_COLORS = {
    "coverall": (221, 104, 38),      # high-visibility orange boiler suit
    "dark_suit": (37, 55, 70),       # industrial blue/navy side panels
    "reflective": (202, 220, 215),
    "skin": (183, 136, 100),
    "helmet": (230, 184, 49),      # visually identifiable hard hat
    "glove": (53, 67, 70),
    "boot": (32, 39, 43),
    "hair": (45, 40, 37),
}
REGION_WORDS = {
    "helmet": ("hardhat", "helmet", "safetyhat", "cap"),
    "glove": ("glove",),
    "boot": ("boot", "shoe"),
    "hair": ("hair", "beard"),
    "skin": ("face", "eyebrow", "nose", "neck", "ear"),
}


def semantic_region(obj, evaluated_mesh, tri, material):
    names = (obj.name + " " + (material.name if material else "")).lower()
    for region, keys in REGION_WORDS.items():
        if any(k in names for k in keys):
            return region
    group_scores = {"skin": 0.0, "glove": 0.0, "boot": 0.0,
                    "coverall": 0.0}
    for vertex_index in tri.vertices:
        vertex = evaluated_mesh.vertices[vertex_index]
        for binding in vertex.groups:
            if binding.group >= len(obj.vertex_groups):
                continue
            name = obj.vertex_groups[binding.group].name.lower()
            weight = binding.weight
            if any(k in name for k in ("finger", "thumb", "hand", "wrist")):
                group_scores["glove"] += weight
            elif any(k in name for k in ("foot", "toe", "ankle")):
                group_scores["boot"] += weight
            elif any(k in name for k in ("head", "neck", "jaw")):
                group_scores["skin"] += weight
            else:
                group_scores["coverall"] += weight
    if group_scores["glove"] > group_scores["coverall"] * .85 and group_scores["glove"] > .03:
        return "glove"
    if group_scores["boot"] > group_scores["coverall"] * .80 and group_scores["boot"] > .03:
        return "boot"
    if group_scores["skin"] > group_scores["coverall"] * .70 and group_scores["skin"] > .03:
        return "skin"
    if "body" in names or "worker" in names or "clothes" in names:
        return "coverall"
    return max(group_scores, key=group_scores.get)


def workwear_color(region, tri, material, obj, mesh):
    rgb = MARINE_COLORS[region]
    # Flat lighting is a visual-only baked material (PSP disables dynamic
    # lights). A modest top/front facet shade gives readable anatomy.
    n = tri.normal
    brightness = n.z * .75 + n.x * .25
    factor = (.77 if brightness < -.45 else
              .88 if brightness < .05 else
              1.00 if brightness < .60 else 1.10)
    # Preserve facial detail without recreating noisy atlas seams.
    if region == "skin" and material is not None:
        source = base_color(material, mesh,
                            mesh.polygons[tri.polygon_index], tri)
        sr, sg, sb = source & 255, (source >> 8) & 255, (source >> 16) & 255
        # Only keep original dark eyes/lashes on very dark atlas texels.
        if max(sr, sg, sb) < 65:
            rgb = (74, 54, 45)
    return rgba_to_abgr(*[min(255, c * factor)/255 for c in rgb])


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
    topology = []
    regions = {}
    for obj in meshes:
        evaluated = obj.evaluated_get(dg)
        evaluated_mesh = evaluated.to_mesh(
            preserve_all_data_layers=True, depsgraph=dg)
        if evaluated_mesh is None:
            raise ValueError(f"Cannot evaluate skinned mesh {obj.name}")
        evaluated_mesh.calc_loop_triangles()
        mats = [slot.material for slot in obj.material_slots]
        for tri in evaluated_mesh.loop_triangles:
            polygon = evaluated_mesh.polygons[tri.polygon_index]
            material = (mats[polygon.material_index]
                        if polygon.material_index < len(mats) else None)
            if palette is None:
                region = semantic_region(obj, evaluated_mesh, tri, material)
                regions[region] = regions.get(region, 0) + 1
                abgr = workwear_color(region, tri, material, obj, evaluated_mesh)
            else:
                abgr = palette[len(color_out)]
            topology.append((obj.name, tuple(tri.vertices)))
            for vertex_index in tri.vertices:
                p = evaluated.matrix_world @ evaluated_mesh.vertices[vertex_index].co
                # Blender +Z vertical / -Y forward -> Engine Watch +Y up / +X forward.
                vertices.append((-p.y, p.z, p.x))
                color_out.append(abgr)
        evaluated.to_mesh_clear()
    # Root motion belongs to the actual C++ walk controller. Use the skinned
    # hips bone, rather than re-centering every frame from animated arm/leg
    # bounding boxes (which previously made the whole body appear jittery).
    root_world = None
    for rig in rigs:
        hips = next((p for p in rig.pose.bones
                     if "hip" in p.name.lower() or "pelvis" in p.name.lower()),
                    None)
        if hips:
            root_world = rig.matrix_world @ hips.matrix.translation
            break
    root = (-root_world.y, root_world.z, root_world.x) if root_world else None
    return vertices, color_out, tuple(topology), root, regions


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
    original, final_tris, lod_report = reduce_model(meshes, args.triangles)
    clips = [("idle", 6), ("walk", 12), ("interact", 8)]
    poses = []
    colors = None
    reference_topology = None
    roots = []
    region_histogram = {}
    for name, frames in clips:
        action = find_action(name)
        for i in range(frames):
            verts, new_colors, topology, root, regions = sample_scene(
                bpy.context.scene, rigs, meshes, action,
                i / float(frames), colors)
            if colors is None:
                colors = new_colors
                region_histogram = regions
                reference_topology = topology
            if len(verts) != final_tris * 3:
                raise ValueError("Animated vertex count changed between samples")
            if topology != reference_topology:
                raise ValueError(
                    "Imported skeleton topology/triangle order changes between "
                    "frames; cannot safely interpolate PSP vertex streams")
            if root is None:
                raise ValueError("No stable Hips/Pelvis bone found for root-motion removal")
            roots.append(root)
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
        for pose, root in zip(poses, roots):
            # Ground foot contact per sample; correct an imported clip's
            # vertical root bounce without moving the game player.
            sample_floor = min(p[1] for p in pose)
            for index, pos in enumerate(pose):
                x = (pos[0] - root[0]) * scale
                y = (pos[1] - sample_floor) * scale
                z = (pos[2] - root[2]) * scale
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
        "mesh_lod_allocation": lod_report,
        "semantic_face_regions": region_histogram,
        "stable_triangle_correspondence": True,
        "root_motion_source": "original Hips bone",
        "foot_floor_alignment": "per baked frame",
        "material_palette": "marine high-vis orange boiler suit, yellow helmet, gloves, boots",
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
