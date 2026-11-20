#!/usr/bin/env python3
"""Validate Engine Watch baked human clip format (EWHM v1), no dependencies."""
from pathlib import Path
import argparse
import json
import math
import struct

HEADER = struct.Struct("<4sIIIIIff")
VERTEX = struct.Struct("<Ifff")


def inspect(path: Path) -> dict:
    blob = path.read_bytes()
    if len(blob) < HEADER.size:
        raise ValueError("EWHM truncated header")
    magic, version, vertices, idle, walk, interact, height, fps = HEADER.unpack_from(blob)
    if magic != b"EWHM" or version != 1:
        raise ValueError("invalid EWHM signature/version")
    if vertices == 0 or vertices % 3 or vertices > 1800:
        raise ValueError("PSP triangle vertex budget exceeded")
    frames = idle + walk + interact
    if not all(0 < x <= 16 for x in (idle, walk, interact)) or frames > 40:
        raise ValueError("invalid animation clip frame counts")
    expected = HEADER.size + frames * vertices * VERTEX.size
    if len(blob) != expected or len(blob) > 1350000:
        raise ValueError("invalid frame data size")
    if not 1.0 <= height <= 2.2 or not 1 <= fps <= 40:
        raise ValueError("invalid human height or sample rate")
    bounds = [[float("inf")] * 3, [float("-inf")] * 3]
    all_frames = []
    offset = HEADER.size
    for k in range(frames):
        positions = []
        for v in range(vertices):
            color, x, y, z = VERTEX.unpack_from(blob, offset)
            offset += VERTEX.size
            if color >> 24 != 255 or not all(map(math.isfinite, (x,y,z))):
                raise ValueError("invalid vertex value")
            if max(abs(x),abs(y),abs(z)) > 4.0:
                raise ValueError("invalid geometry bounds")
            for n, val in enumerate((x, y, z)):
                bounds[0][n] = min(bounds[0][n], val)
                bounds[1][n] = max(bounds[1][n], val)
            positions.append((x,y,z))
        all_frames.append(positions)
    if bounds[1][1] - bounds[0][1] < 1.45:
        raise ValueError("humanoid height lost during simplification")
    # In a real skinned source, Walk differs from Idle at multiple points.
    idle_pose, walk_pose = all_frames[0], all_frames[idle + walk // 4]
    diff = sum(sum((a-b)**2 for a,b in zip(p,q)) > 1e-5
               for p,q in zip(idle_pose,walk_pose))
    if diff < max(3, vertices // 30):
        raise ValueError("Walk animation appears static; source rig not baked")
    return {
        "format": "EngineWatch EWHM v1",
        "size_bytes": len(blob),
        "vertices_per_frame": vertices,
        "triangles_per_frame": vertices // 3,
        "clips": {"idle": idle, "walk": walk, "interact": interact},
        "height_m": round(height,3),
        "fps": fps,
        "animated_vertices_vs_idle": diff,
        "bounds_xyz": bounds
    }


def main():
    p = argparse.ArgumentParser()
    p.add_argument("file", type=Path)
    p.add_argument("--report", type=Path)
    args = p.parse_args()
    result = inspect(args.file)
    if args.report:
        args.report.parent.mkdir(parents=True, exist_ok=True)
        args.report.write_text(json.dumps(result, indent=2) + "\n")
    print("EWHM VALIDATION PASS")
    print(json.dumps(result, indent=2))


if __name__ == "__main__":
    main()
