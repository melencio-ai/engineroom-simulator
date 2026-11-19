#!/usr/bin/env python3
"""G2b modular marine assembly exporter: joint BOM, GLB, OBJ, PSP and viewer.

Each registered part is generated independently. Instances are transformed
from local millimetre coordinates into an assembly, without changing source
mesh definitions. Everything is illustrative / NOT for fabrication.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import math
import struct
from collections import Counter
from pathlib import Path

from assembly_builder import (
    bill_of_materials,local_to_world,transform_vector,validate_mated_assembly,
)
from export_catalog import (
    MATERIALS, face_normal, flatten, psp_vertex_stream, triangles_to_mtl,
)
from geometry import Mesh,build_mesh
from validate_catalog import ROOT,read_json,validate_catalog

MAX_PSP_CHUNK_TRIANGLES=1800
DISCLAIMER="ILLUSTRATIVE model only; not pressure-rated, surveyed, or approved for shipbuilding."


def hexhash(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def transform_mesh(mesh: Mesh,child: dict,extra=(0.0,0.0,0.0)) -> Mesh:
    out=Mesh()
    for tri in mesh.triangles:
        out.tri(*(local_to_world(v,child,extra) for v in tri))
    if out.triangle_count != mesh.triangle_count:
        raise ValueError(f"{child['instance_id']}: transformation lost triangles")
    return out


def moved(child: dict,steps: dict) -> dict:
    new=dict(child)
    displacement=steps.get(child["instance_id"],(0.0,0.0,0.0))
    new["translation_mm"]=[
        child["translation_mm"][i]+displacement[i] for i in range(3)
    ]
    return new


def make_matrix(child: dict) -> list[float]:
    """glTF column-major matrix with Euler X/Y/Z in degrees."""
    e1=transform_vector((1,0,0),child["rotation_deg"])
    e2=transform_vector((0,1,0),child["rotation_deg"])
    e3=transform_vector((0,0,1),child["rotation_deg"])
    x,y,z=child["translation_mm"]
    return [*e1,0.0,*e2,0.0,*e3,0.0,float(x),float(y),float(z),1.0]


def glb_from_parts(asm: dict,meshes: dict,records: dict,steps: dict) -> bytes:
    """Valid glTF 2.0 GLB with individual, selectable named child nodes."""
    data=bytearray()
    views=[]
    accessors=[]
    primitives={}
    materials=[]
    material_names={}
    for comp_id in sorted(meshes):
        mesh=meshes[comp_id]
        part=records[comp_id]
        material=part["material"]["base_material"]
        if material not in material_names:
            colour=MATERIALS[material]
            material_names[material]=len(materials)
            materials.append({
                "name":"illustrative "+material,
                "doubleSided":True,
                "pbrMetallicRoughness":{
                    "baseColorFactor":[*(c/255 for c in colour),1.0],
                    "metallicFactor":0.32 if material=="carbon_steel" else 0.02,
                    "roughnessFactor":0.76}
            })
        pos=flatten(mesh)
        nrm=[]
        for tri in mesh.triangles:
            n=face_normal(*tri)
            nrm.extend([n,n,n])
        parts=[pos,nrm]
        acc=[]
        for attr,vectors in enumerate(parts):
            while len(data)%4: data.append(0)
            offset=len(data)
            data.extend(b"".join(struct.pack("<fff",*v) for v in vectors))
            views.append({
                "buffer":0,"byteOffset":offset,
                "byteLength":len(data)-offset,"target":34962
            })
            vals={"bufferView":len(views)-1,"componentType":5126,
                  "count":len(vectors),"type":"VEC3"}
            if attr==0:
                lo,hi=mesh.bounds()
                vals["min"]=list(lo);vals["max"]=list(hi)
            accessors.append(vals)
            acc.append(len(accessors)-1)
        primitives[comp_id]={
            "attributes":{"POSITION":acc[0],"NORMAL":acc[1]},
            "material":material_names[material],"mode":4}
    ids=sorted(primitives)
    mesh_index={x:i for i,x in enumerate(ids)}
    nodes=[]
    for child in asm["children"]:
        positioned=moved(child,steps)
        nodes.append({
            "name":child["instance_id"],
            "mesh":mesh_index[child["component_id"]],
            "matrix":make_matrix(positioned),
            "extras":{
                "instance_id":child["instance_id"],
                "component_id":child["component_id"],
                "maintainable":child["maintainable"]}
        })
    doc={
        "asset":{"version":"2.0","generator":"Engine Watch G2b assemblies"},
        "scene":0,"scenes":[{"nodes":list(range(len(nodes))),
                            "name":asm["name"]}],
        "nodes":nodes,
        "meshes":[{"name":x,"primitives":[primitives[x]]} for x in ids],
        "materials":materials,
        "buffers":[{"byteLength":len(data)}],
        "bufferViews":views,"accessors":accessors,
        "extras":{
            "engineering_status":"illustrative",
            "not_for_fabrication":True,
            "assembly_id":asm["id"],
            "units":"mm",
            "disclaimer":DISCLAIMER
        }
    }
    j=json.dumps(doc,sort_keys=True,separators=(",",":")).encode("utf-8")
    j+=b" "*((-len(j))%4)
    binary=bytes(data)+b"\x00"*((-len(data))%4)
    total=12+8+len(j)+8+len(binary)
    return (struct.pack("<4sII",b"glTF",2,total)
            +struct.pack("<I4s",len(j),b"JSON")+j
            +struct.pack("<I4s",len(binary),b"BIN\x00")+binary)


def assembly_obj(asm: dict,meshes: dict,records: dict,steps: dict) -> tuple[bytes,bytes]:
    lines=[
        "# Engine Watch assembly; coordinates millimetres; illustrative only",
        f"# {DISCLAIMER}",
        f"mtllib {asm['id']}.mtl"]
    mats=set()
    index=1
    for child in asm["children"]:
        comp_id=child["component_id"]
        material=records[comp_id]["material"]["base_material"]
        mats.add(material)
        mesh=transform_mesh(meshes[comp_id],moved(child,steps))
        lines.append("o "+child["instance_id"])
        lines.append("usemtl "+material)
        for tri in mesh.triangles:
            lines.extend("v %.7f %.7f %.7f"%point for point in tri)
            lines.append(f"f {index} {index+1} {index+2}")
            index+=3
    mtl=b"# Illustration-only material palette\n"
    for material in sorted(mats):
        mtl+=triangles_to_mtl(MATERIALS[material],material)
    return ("\n".join(lines)+"\n").encode("utf-8"),mtl


def psp_chunks(asm: dict,meshes: dict,records: dict,
               out_dir: Path) -> list[dict]:
    out_dir.mkdir(parents=True,exist_ok=True)
    chunks=[]
    pending=Mesh()
    references=[]
    def flush() -> None:
        nonlocal pending,references
        if not pending.triangles: return
        n=len(chunks)
        rel=f"psp/chunk-{n:03d}.ewmesh"
        # A per-chunk neutral metal colour is illustrative; proper per-part
        # colour is serialized below by manually replacing each vertex tint.
        blob=bytearray(struct.pack("<4sII",b"EW3D",1,pending.triangle_count*3))
        for rec in references:
            section=rec["mesh"]
            material=rec["material"]
            r,g,b=MATERIALS[material]
            abgr=0xff000000|(b<<16)|(g<<8)|r
            for vx,vy,vz in flatten(section):
                blob.extend(struct.pack("<Ifff",abgr,vx/1000,vy/1000,vz/1000))
        expected=12+pending.triangle_count*3*16
        if len(blob)!=expected:
            raise RuntimeError("PSP chunk size disagrees with triangle count")
        (out_dir/f"chunk-{n:03d}.ewmesh").write_bytes(blob)
        chunks.append({
            "path":rel,"sha256":hexhash(blob),
            "triangles":pending.triangle_count,
            "instances":[{"id":rec["id"],
                          "triangles":rec["mesh"].triangle_count}
                         for rec in references]
        })
        pending=Mesh();references=[]
    for child in asm["children"]:
        source=meshes[child["component_id"]]
        mesh=transform_mesh(source,child)
        n=mesh.triangle_count
        if n>MAX_PSP_CHUNK_TRIANGLES:
            raise ValueError("One part exceeds PSP batch budget")
        if pending.triangle_count+n>MAX_PSP_CHUNK_TRIANGLES:
            flush()
        pending.triangles.extend(mesh.triangles)
        references.append({"id":child["instance_id"],"mesh":mesh,
                           "material":records[child["component_id"]]["material"]["base_material"]})
    flush()
    if sum(x["triangles"] for x in chunks)!=sum(
        meshes[c["component_id"]].triangle_count for c in asm["children"]
    ):
        raise RuntimeError("Combined PSP assembly lost geometry")
    return chunks


def viewer_payload(asm: dict,meshes: dict,records: dict) -> dict:
    out={}
    for comp_id in sorted(meshes):
        mesh=meshes[comp_id]
        normals=[]
        for tri in mesh.triangles:
            n=face_normal(*tri)
            normals.extend([*n]*3)
        out[comp_id]={
            "positions":[round(x,5) for v in flatten(mesh) for x in v],
            "normals":normals,
            "rgb":[c/255 for c in MATERIALS[
                records[comp_id]["material"]["base_material"]]],
            "display_name":records[comp_id]["name"],
            "material":records[comp_id]["material"]["base_material"],
            "parameters":records[comp_id]["parameters"]
        }
    steps={s["instance_id"]:s for s in asm["exploded_steps"]}
    # Fit the initial orbit camera to actual assembled-world geometry. Without
    # this, a 60-mm bolt kit is almost invisible using a 2-metre pipe-camera.
    low=[float("inf")]*3
    high=[float("-inf")]*3
    for child in asm["children"]:
        a,b=meshes[child["component_id"]].bounds()
        for x in (a[0],b[0]):
            for y in (a[1],b[1]):
                for z in (a[2],b[2]):
                    world=local_to_world((x,y,z),child)
                    for i in range(3):
                        low[i]=min(low[i],world[i])
                        high[i]=max(high[i],world[i])
    center=[(a+b)/2 for a,b in zip(low,high)]
    extent=max(b-a for a,b in zip(low,high))
    camera={
        "target":center,
        "distance":max(160,extent*1.50),
        "min_distance":max(32,extent*0.12),
        "max_distance":max(600,extent*5.0)
    }
    return {
        "assembly":asm["id"],"title":asm["name"],"camera":camera,
        "notice":DISCLAIMER,
        "parts":out,
        "instances":[{
            "id":c["instance_id"],"component_id":c["component_id"],
            "translation":c["translation_mm"],
            "rotation":c["rotation_deg"],
            "maintainable":c["maintainable"],
            "exploded":steps.get(c["instance_id"],{}).get(
                "translation_mm",[0,0,0]),
            "step":steps.get(c["instance_id"],{}).get("step",0)
        } for c in asm["children"]],
        "maxStep":max((s["step"] for s in asm["exploded_steps"]),default=0),
        "bom":bill_of_materials(asm),
        "connections":asm["connections"]
    }


def export_assemblies(root: Path,output: Path,only: set[str]|None=None) -> dict:
    validate_catalog(root)
    registry=read_json(root/"catalog/registry.json")
    sources={Path(p).stem:p for p in registry["components"]}
    component={id:read_json(root/path) for id,path in sources.items()}
    assembly_paths={Path(p).stem:p for p in registry["assemblies"]}
    if only:
        unknown=only-set(assembly_paths)
        if unknown: raise ValueError("Unknown assembly IDs: "+", ".join(sorted(unknown)))
    output.mkdir(parents=True,exist_ok=True)
    records=[]
    deferred=[]
    template=(root/"tools/asset_pipeline/assembly_viewer_template.html").read_text(encoding="utf-8")
    for ident in sorted(assembly_paths):
        if only and ident not in only: continue
        asm=read_json(root/assembly_paths[ident])
        if not all(c["component_id"] in component for c in asm["children"]):
            deferred.append(ident)
            continue  # G1's unresolved motor/pump TODO is not a real assembly.
        details=validate_mated_assembly(asm,component)
        unique=sorted({c["component_id"] for c in asm["children"]})
        hi={id:build_mesh(component[id],"inspection") for id in unique}
        low={id:build_mesh(component[id],"psp") for id in unique}
        steps={s["instance_id"]:tuple(s["translation_mm"])
               for s in asm["exploded_steps"]}
        dest=output/ident
        dest.mkdir(parents=True,exist_ok=True)
        files={}
        def save(name,data):
            path=dest/name
            path.parent.mkdir(parents=True,exist_ok=True)
            path.write_bytes(data)
            files[name]=hexhash(data)
        save("assembled.glb",glb_from_parts(asm,hi,component,{}))
        save("exploded.glb",glb_from_parts(asm,hi,component,steps))
        for label,shifts in (("assembled",{}),("exploded",steps)):
            obj,mtl=assembly_obj(asm,hi,component,shifts)
            save(f"{label}.obj",obj)
            save(f"{asm['id']}.mtl",mtl)
        chunks=psp_chunks(asm,low,component,dest/"psp")
        for chunk in chunks: files[chunk["path"]]=chunk["sha256"]
        scene=viewer_payload(asm,hi,component)
        safe=json.dumps(scene,sort_keys=True,separators=(",",":")).replace("<","\\u003c").replace("&","\\u0026")
        if template.count("__ENGINEWATCH_ASSEMBLY_DATA__")!=1:
            raise RuntimeError("Viewer template must have one data placeholder")
        save("viewer.html",template.replace("__ENGINEWATCH_ASSEMBLY_DATA__",safe).encode("utf-8"))
        manifest={
            "id":ident,"status":asm["status"],
            "warning":DISCLAIMER,"engineering_approved":False,
            "source":assembly_paths[ident],
            "source_sha256":hexhash((root/assembly_paths[ident]).read_bytes()),
            "bom":bill_of_materials(asm),
            "instance_count":len(asm["children"]),
            "inspectable_instances":[x["instance_id"] for x in asm["children"]],
            "resolved_connections":len(asm["connections"]),
            "exploded_steps":len(asm["exploded_steps"]),
            "joint_fit":details,"psp_chunks":chunks,
            "glb_units":"mm","psp_units":"m",
            "outputs_sha256":files
        }
        save("manifest.json",(json.dumps(manifest,sort_keys=True,indent=2)+"\n").encode("utf-8"))
        records.append({"id":ident,"instances":len(asm["children"]),
                        "psp_chunks":len(chunks),"relative_path":ident})
    summary={
        "schema_version":"1.0.0","generated":records,
        "deferred_unresolved":deferred,"warnings":[DISCLAIMER]
    }
    (output/"assemblies.json").write_text(json.dumps(summary,sort_keys=True,indent=2)+"\n",
                                          encoding="utf-8")
    return summary


def main() -> None:
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("--root",type=Path,default=ROOT)
    p.add_argument("--out",type=Path,default=ROOT/"dist/parametric-assemblies")
    p.add_argument("--id",action="append")
    args=p.parse_args()
    summary=export_assemblies(args.root,args.out,set(args.id) if args.id else None)
    for rec in summary["generated"]:
        print(f"G2b assembly {rec['id']}: {rec['instances']} parts / "
              f"{rec['psp_chunks']} PSP chunks")
    print("Deferred unresolved draft assemblies: "+", ".join(summary["deferred_unresolved"]))
    print("ASSEMBLY EXPORT PASS: "+str(len(summary["generated"]))+" resolved assemblies")


if __name__=="__main__":
    main()
