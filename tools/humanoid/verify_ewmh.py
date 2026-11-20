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
    if vertices == 0 or vertices % 3 or vertices > 3000:
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
    colors_first_frame = []
    offset = HEADER.size
    for k in range(frames):
        positions = []
        for v in range(vertices):
            color, x, y, z = VERTEX.unpack_from(blob, offset)
            offset += VERTEX.size
            if color >> 24 != 255 or not all(map(math.isfinite, (x,y,z))):
                raise ValueError("invalid vertex value")
            if k == 0:
                colors_first_frame.append(color)
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
    # Do not silently accept a 900-triangle mesh with shredded faces or
    # mismatched indices across skeletal poses. The cheap C++ runtime will
    # interpolate corresponding triangle vertices and cannot repair these.
    from collections import Counter
    unique_colors = len(set(colors_first_frame))
    if unique_colors > 100:
        raise ValueError(
            f"Broken atlas/colour sampling: {unique_colors} tiny face colours")
    if unique_colors < 3 and vertices >= 1200:
        raise ValueError("Marine PPE palette is not present")
    degenerate = 0
    longest_edge = 0.0
    frame_heights = []
    max_frame_step = 0.0
    for frame in all_frames:
        ys = [v[1] for v in frame]
        height = max(ys) - min(ys)
        frame_heights.append(height)
        if not 1.45 <= height <= 2.1:
            raise ValueError(f"An animation distorted the body height: {height:.3f}m")
        for i in range(0, vertices, 3):
            a, b, c = frame[i:i+3]
            ab = tuple(b[j]-a[j] for j in range(3))
            ac = tuple(c[j]-a[j] for j in range(3))
            bc = tuple(c[j]-b[j] for j in range(3))
            for edge in (ab, ac, bc):
                longest_edge = max(longest_edge,
                                   math.sqrt(sum(v*v for v in edge)))
            cross = (ab[1]*ac[2]-ab[2]*ac[1],
                     ab[2]*ac[0]-ab[0]*ac[2],
                     ab[0]*ac[1]-ab[1]*ac[0])
            if sum(v*v for v in cross) < 1e-12:
                degenerate += 1
    total_triangles = (frames * vertices) // 3
    degenerate_ratio = degenerate/max(1, total_triangles)
    if degenerate_ratio > .025:
        raise ValueError(
            f"Too many broken/zero-area triangles: {degenerate_ratio:.1%}")
    if longest_edge > 1.35:
        raise ValueError(f"Likely damaged polygon spans {longest_edge:.2f} metres")

    # Adjacent animation samples should not explode/jump from unrelated
    # triangles. Clip changes are excluded: they are blended by the game.
    boundaries = [(0, idle), (idle, idle+walk),
                  (idle+walk, frames)]
    for start, end in boundaries:
        for n in range(start+1, end):
            prev, curr = all_frames[n-1], all_frames[n]
            for a, b in zip(prev, curr):
                jump = math.sqrt(sum((a[j]-b[j])**2 for j in range(3)))
                max_frame_step = max(max_frame_step, jump)
    if max_frame_step > .87:
        raise ValueError(
            f"Broken skeletal frame correspondence: {max_frame_step:.2f}m jump")

    return {
        "format": "EngineWatch EWHM v1",
        "size_bytes": len(blob),
        "vertices_per_frame": vertices,
        "triangles_per_frame": vertices // 3,
        "clips": {"idle": idle, "walk": walk, "interact": interact},
        "height_m": round(height,3),
        "fps": fps,
        "animated_vertices_vs_idle": diff,
        "unique_palette_colors": unique_colors,
        "degenerate_triangle_ratio": round(degenerate_ratio, 6),
        "longest_triangle_edge_m": round(longest_edge, 4),
        "largest_adjacent_frame_vertex_step_m": round(max_frame_step, 4),
        "animation_height_range_m": [round(min(frame_heights), 3),
                                     round(max(frame_heights), 3)],
        "stable_face_colours": all(
            colors_first_frame[i] == colors_first_frame[i+1] ==
            colors_first_frame[i+2]
            for i in range(0, vertices, 3)
        ),
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
