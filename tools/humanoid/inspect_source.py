#!/usr/bin/env python3
"""Check the exact pinned CC0 Quaternius Worker GLB before Blender conversion.

No asset download is implicit. CI fetches the individually converted
Worker.glb and verifies the Git blob hash advertised by GitHub.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct

BASE = Path(__file__).resolve().parent


def inspect(path: Path, license_meta: dict) -> dict:
    blob = path.read_bytes()
    if len(blob) < 48 or blob[:4] != b"glTF":
        raise ValueError("Not a glTF 2.0 binary (GLB) file")
    magic, version, total = struct.unpack_from("<4sII", blob, 0)
    if version != 2 or total != len(blob):
        raise ValueError("Corrupt/incompatible GLB header")
    git_hash = hashlib.sha1(
        b"blob " + str(len(blob)).encode("ascii") + b"\0" + blob
    ).hexdigest()
    if git_hash != license_meta["model_git_blob_sha1"]:
        raise ValueError(
            "Source mismatch: expected immutable Git blob "
            f"{license_meta['model_git_blob_sha1']}, got {git_hash}"
        )
    json_len, chunk_type = struct.unpack_from("<I4s", blob, 12)
    if chunk_type != b"JSON":
        raise ValueError("GLB JSON chunk missing")
    gltf = json.loads(blob[20:20 + json_len])
    if gltf.get("asset", {}).get("version") != "2.0":
        raise ValueError("Expected glTF 2.0 source")
    animations = [a.get("name", f"clip_{i}")
                  for i, a in enumerate(gltf.get("animations", []))]
    meshes = gltf.get("meshes", [])
    if not meshes or not gltf.get("skins") or not animations:
        raise ValueError("Source Worker is not an animated, rigged humanoid")
    # The source conversion repository explicitly supplies Idle/Walk/Interact.
    # Fail instead of producing a static placeholder when it changes.
    for required in ("idle", "walk", "interact"):
        if not any(required in n.lower() for n in animations):
            raise ValueError(f"Required source animation {required!r} missing")
    accessors = gltf.get("accessors", [])
    original_triangles = 0
    for mesh in meshes:
        for primitive in mesh.get("primitives", []):
            if primitive.get("mode", 4) != 4:
                raise ValueError("Source has non-triangle primitives")
            index = primitive.get("indices")
            if index is not None:
                original_triangles += accessors[index]["count"] // 3
            else:
                original_triangles += accessors[
                    primitive["attributes"]["POSITION"]]["count"] // 3
    if original_triangles < 100:
        raise ValueError("Source does not contain recognizable humanoid mesh")
    return {
        "source_id": license_meta["id"],
        "creator": "Quaternius",
        "source_pack": license_meta["source_pack_page"],
        "mirror": license_meta["archive_mirror_project"],
        "sha1_git_blob": git_hash,
        "bytes": len(blob),
        "mesh_count": len(meshes),
        "rig_count": len(gltf["skins"]),
        "animations": animations,
        "original_triangle_count": original_triangles,
        "source_license": license_meta["license"],
        "note": "Unmodified Worker.glb is NOT committed to the Engine Watch repository.",
    }


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--source", required=True, type=Path)
    ap.add_argument("--report", type=Path)
    args = ap.parse_args()
    meta = json.loads((BASE / "quaternius_worker_source.json").read_text())
    report = inspect(args.source, meta)
    print(json.dumps(report, indent=2))
    if args.report:
        args.report.parent.mkdir(parents=True, exist_ok=True)
        args.report.write_text(json.dumps(report, indent=2) + "\n")


if __name__ == "__main__":
    main()
