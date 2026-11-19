#!/usr/bin/env python3
"""Export validated illustrative part records as OBJ/MTL, GLB and PSP mesh.

* High-detail inspection meshes are triangle geometry in MILLIMETRES.
* PSP EW3D binaries are colour+XYZ triangles in METRES, matching Vertex3D.
* Standards conformity, pressure rating and physical thread fit NOT certified.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import math
import struct
from pathlib import Path

from geometry import Mesh, build_mesh
from validate_catalog import ROOT, read_json, validate_catalog

MAGIC=b"EW3D"
VERSION=1
MATERIALS={
    "carbon_steel": (154,168,168),
    "fiber_sheet": (65,74,69),
}
# Important: these are presentation colours only, never physical material IDs.


def colour_abgr(rgb: tuple[int,int,int]) -> int:
    r,g,b=rgb
    return 0xff000000 | (b<<16) | (g<<8) | r


def sha256(content: bytes) -> str:
    return hashlib.sha256(content).hexdigest()


def flatten(mesh: Mesh) -> list[tuple[float,float,float]]:
    return [p for tri in mesh.triangles for p in tri]


def triangles_to_obj(mesh: Mesh, ident: str, material: str,
                     comment: str) -> bytes:
    out=[
        "# Engine Watch 3D independent parametric component",
        "# Units: MILLIMETRES; dimensions ILLUSTRATIVE and NOT standards-certified",
        f"# ID: {ident}",
        f"# {comment}",
        f"mtllib {ident}.mtl",
        f"o {ident}",
        f"usemtl {material}",
    ]
    vertices=flatten(mesh)
    out.extend(f"v {x:.7f} {y:.7f} {z:.7f}" for x,y,z in vertices)
    out.extend(f"f {i} {i+1} {i+2}" for i in range(1,len(vertices)+1,3))
    return ("\n".join(out)+"\n").encode("utf-8")


def triangles_to_mtl(rgb: tuple[int,int,int], name: str) -> bytes:
    r,g,b=(v/255 for v in rgb)
    return (f"# Illustrative visual material only, not material grade\n"
            f"newmtl {name}\nKa 0.150000 0.150000 0.150000\n"
            f"Kd {r:.6f} {g:.6f} {b:.6f}\nKs 0.080000 0.080000 0.080000\n"
            f"Ns 16\nillum 2\n").encode("utf-8")


def face_normal(a,b,c):
    ux,uy,uz=(b[i]-a[i] for i in range(3))
    vx,vy,vz=(c[i]-a[i] for i in range(3))
    x,y,z=uy*vz-uz*vy,uz*vx-ux*vz,ux*vy-uy*vx
    n=math.sqrt(x*x+y*y+z*z)
    return x/n,y/n,z/n


def inspection_glb(mesh: Mesh, part: dict,
                   rgb: tuple[int,int,int]) -> bytes:
    """Minimal valid glTF 2.0 binary triangle mesh with flat face normals."""
    positions=flatten(mesh)
    normals=[]
    for tri in mesh.triangles:
        normal=face_normal(*tri)
        normals.extend([normal,normal,normal])
    pb=b"".join(struct.pack("<fff",*p) for p in positions)
    nb=b"".join(struct.pack("<fff",*p) for p in normals)
    binary=pb+nb
    low,high=mesh.bounds()
    gltf={
        "asset":{"version":"2.0","generator":"Engine Watch parametric inspection v0.2"},
        "extensionsUsed":[],
        "scene":0,
        "scenes":[{"nodes":[0]}],
        "nodes":[{"mesh":0,"name":part["id"]}],
        "meshes":[{"name":part["id"],"primitives":[{
            "attributes":{"POSITION":0,"NORMAL":1},"material":0,"mode":4
        }]}],
        "materials":[{
            "name":"illustrative visual finish",
            "doubleSided":True,
            "pbrMetallicRoughness":{
                "baseColorFactor":[*[v/255 for v in rgb],1.0],
                "metallicFactor":0.12 if part["material"]["base_material"]=="fiber_sheet" else 0.36,
                "roughnessFactor":0.77
            }
        }],
        "buffers":[{"byteLength":len(binary)}],
        "bufferViews":[
            {"buffer":0,"byteOffset":0,"byteLength":len(pb),"target":34962},
            {"buffer":0,"byteOffset":len(pb),"byteLength":len(nb),"target":34962}
        ],
        "accessors":[
            {"bufferView":0,"componentType":5126,"count":len(positions),
             "type":"VEC3","min":list(low),"max":list(high)},
            {"bufferView":1,"componentType":5126,"count":len(normals),
             "type":"VEC3"}
        ],
        "extras":{
            "engineering_status":part["status"],
            "units":"mm",
            "certified":False,
            "not_for_fabrication":True,
            "source_id":part["id"]
        }
    }
    json_bytes=json.dumps(gltf,sort_keys=True,separators=(",",":"),
                          ensure_ascii=True).encode("utf-8")
    json_bytes+=b" " * ((-len(json_bytes))%4)
    binary+=b"\x00"*((-len(binary))%4)
    total=12+8+len(json_bytes)+8+len(binary)
    return (struct.pack("<4sII",b"glTF",2,total)
            + struct.pack("<I4s",len(json_bytes),b"JSON")
            + json_bytes
            + struct.pack("<I4s",len(binary),b"BIN\x00")
            + binary)


def psp_vertex_stream(mesh: Mesh, rgb: tuple[int,int,int]) -> bytes:
    """EW3D v1: magic 4B, version u32, count u32, then ABGR8888+XYZ metre."""
    data=bytearray(struct.pack("<4sII",MAGIC,VERSION,mesh.triangle_count*3))
    colour=colour_abgr(rgb)
    for x,y,z in flatten(mesh):
        data.extend(struct.pack("<Ifff",colour,x/1000.0,y/1000.0,z/1000.0))
    return bytes(data)


def export_all(root: Path, dest: Path, selected: set[str] | None = None) -> dict:
    validate_catalog(root)
    registry=read_json(root/"catalog/registry.json")
    entries=[]
    active={Path(x).stem:x for x in registry["components"]}
    if selected is not None:
        missing=selected-set(active)
        if missing:
            raise ValueError("Unknown catalog IDs: "+", ".join(sorted(missing)))
    for part_id in sorted(active):
        if selected is not None and part_id not in selected:
            continue
        part=read_json(root/active[part_id])
        rgb=MATERIALS.get(part["material"]["base_material"])
        if rgb is None:
            raise ValueError("Unconfigured presentation material "+part_id)
        detailed=build_mesh(part,"inspection")
        psp=build_mesh(part,"psp")
        if psp.triangle_count>1800:
            raise ValueError(f"PSP LOD exceeded per-part 1800 triangle limit: {part_id}")
        if psp.triangle_count> detailed.triangle_count:
            raise ValueError(f"PSP variant is more detailed than inspection: {part_id}")
        if psp.triangle_count*3>65535:
            raise ValueError("16-bit batch vertex budget exceeded")
        material="visual_"+part["material"]["base_material"]
        comment="status="+part["status"]+"; detailed polygon model not verified CAD"
        outputs={
            f"inspection/{part_id}.obj":triangles_to_obj(detailed,part_id,material,comment),
            f"inspection/{part_id}.mtl":triangles_to_mtl(rgb,material),
            f"inspection/{part_id}.glb":inspection_glb(detailed,part,rgb),
            f"psp/{part_id}.obj":triangles_to_obj(psp,part_id,material,"PSP LOD visual reference"),
            f"psp/{part_id}.mtl":triangles_to_mtl(rgb,material),
            f"psp/{part_id}.ewmesh":psp_vertex_stream(psp,rgb),
        }
        hashes={}
        for rel,blob in outputs.items():
            path=dest/rel
            path.parent.mkdir(parents=True,exist_ok=True)
            path.write_bytes(blob)
            hashes[rel]=sha256(blob)
        source_blob=(root/active[part_id]).read_bytes()
        low,high=detailed.bounds()
        entries.append({
            "id":part_id,"generator":part["generator"]["name"],
            "source_record":active[part_id],
            "source_sha256":sha256(source_blob),
            "engineering_status":part["status"],
            "engineering_verified":part["provenance"]["verified"],
            "units_master":"mm","units_psp":"metres",
            "inspection_triangles":detailed.triangle_count,
            "psp_triangles":psp.triangle_count,
            "bounds_mm":{"min":list(low),"max":list(high)},
            "files_sha256":hashes
        })
    manifest={
        "schema_version":"1.0.0",
        "tool":"Engine Watch deterministic G2 parametric exporter",
        "warning":"ILLUSTRATIVE dimensions. Not classified, not for fabrication or pressure design.",
        "mesh_version":"EW3D/1", "component_count":len(entries),
        "components":entries
    }
    (dest/"manifest.json").write_text(json.dumps(manifest,indent=2,sort_keys=True)+"\n",
                                      encoding="utf-8")
    return manifest


def main() -> None:
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root",type=Path,default=ROOT)
    parser.add_argument("--out",type=Path,default=ROOT/"dist/parametric-catalog")
    parser.add_argument("--id",action="append",default=None,
                        help="Export a specific component ID (repeatable)")
    args=parser.parse_args()
    manifest=export_all(args.root,args.out,set(args.id) if args.id else None)
    for part in manifest["components"]:
        print(f"{part['id']}: inspection={part['inspection_triangles']} triangles, "
              f"PSP={part['psp_triangles']} triangles")
    print(f"G2 export PASS: {manifest['component_count']} components -> {args.out}")
    print("All models are illustrative; no approved engineering geometry is claimed.")


if __name__=="__main__":
    main()
