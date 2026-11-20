"""Render headless orthographic QA snapshots from the *actual* PSP EWHM.

Unlike a render of the source GLB, this visualizes the exact decimated,
coloured triangle positions shipped in ENGINEER.EWM. Use for PR review.
"""
import argparse
import json
from pathlib import Path
import struct
import sys

import bpy
from mathutils import Vector

HEADER = struct.Struct("<4sIIIIIff")
VERTEX = struct.Struct("<Ifff")


def read_frame(blob, frame, count):
    offset = HEADER.size + frame * count * VERTEX.size
    return list(VERTEX.iter_unpack(blob[offset:offset + count*VERTEX.size]))


def material_for(abgr, existing):
    if abgr not in existing:
        red=(abgr&255)/255.0
        green=((abgr>>8)&255)/255.0
        blue=((abgr>>16)&255)/255.0
        mat=bpy.data.materials.new("PPE_%08X"%abgr)
        mat.diffuse_color=(red,green,blue,1.0)
        existing[abgr]=mat
    return existing[abgr]


def mesh_for(name, frame, materials):
    verts=[(x,z,y) for c,x,y,z in frame]
    tris=[(i,i+1,i+2) for i in range(0,len(verts),3)]
    mesh=bpy.data.meshes.new(name)
    mesh.from_pydata(verts,[],tris)
    mesh.update()
    obj=bpy.data.objects.new(name,mesh)
    bpy.context.collection.objects.link(obj)
    palette={}
    for i,face in enumerate(mesh.polygons):
        key=frame[i*3][0]
        if key not in palette:
            mat=material_for(key,materials)
            palette[key]=len(mesh.materials)
            mesh.materials.append(mat)
        face.material_index=palette[key]
    return obj


def render_view(scene, obj, out_file, viewpoint):
    points={
        "front":(3.8,0,1.1),  # positive game X is the front
        "rear":(-3.8,0,1.1),
        "side":(0,-3.8,1.1),
    }
    camera=scene.camera
    camera.location=points[viewpoint]
    direction=Vector((0,0,1.02))-camera.location
    camera.rotation_euler=direction.to_track_quat("-Z","Y").to_euler()
    scene.render.filepath=str(out_file)
    bpy.ops.render.render(write_still=True)
    print("QA_PREVIEW:",out_file)


def run():
    argv=sys.argv[sys.argv.index("--")+1:] if "--" in sys.argv else []
    p=argparse.ArgumentParser()
    p.add_argument("--input",required=True,type=Path)
    p.add_argument("--output",required=True,type=Path)
    args=p.parse_args(argv)
    raw=args.input.read_bytes()
    magic,version,count,idle,walk,interact,height,rate=HEADER.unpack_from(raw)
    if magic!=b"EWHM" or version!=1 or count%3 or count>3000:
        raise ValueError("Invalid PSP avatar bank for visual preview")
    bpy.ops.wm.read_factory_settings(use_empty=True)
    scene=bpy.context.scene
    scene.render.engine="BLENDER_WORKBENCH"
    scene.render.resolution_x=440
    scene.render.resolution_y=600
    scene.render.resolution_percentage=100
    scene.render.image_settings.file_format="PNG"
    scene.display.shading.light="STUDIO"
    scene.display.shading.color_type="MATERIAL"
    scene.display.shading.show_cavity=True
    scene.display.shading.background_type="WORLD"
    scene.display.shading.background_color=(0.80,0.84,0.87)
    cam_data=bpy.data.cameras.new("PSP_Review_Ortho")
    camera=bpy.data.objects.new("PSP_Review_Ortho",cam_data)
    bpy.context.collection.objects.link(camera)
    camera.data.type="ORTHO"
    camera.data.ortho_scale=2.24
    scene.camera=camera
    args.output.mkdir(parents=True,exist_ok=True)
    materials={}
    shots=[
        ("01_idle_front",0,"front"),
        ("02_idle_rear",0,"rear"),
        ("03_walk_side",idle+walk//4,"side"),
        ("04_interact_front",idle+walk+interact//2,"front"),
    ]
    outputs=[]
    for name,frame,view in shots:
        person=mesh_for(name,read_frame(raw,frame,count),materials)
        filename=args.output/(name+".png")
        render_view(scene,person,filename,view)
        outputs.append(str(filename))
        bpy.data.objects.remove(person,do_unlink=True)
    (args.output/"README.txt").write_text(
        "These renders show the exact 3D triangle frames inside the PSP "
        "ENGINEER.EWM, not the original high-poly GLB. "
        "Compare head, boots, arms, workwear and helmet against the game.\n")
    print("QUALITY_PREVIEW_PASS",json.dumps(outputs))


if __name__=="__main__":
    run()
