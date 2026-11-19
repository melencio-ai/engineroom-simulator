#!/usr/bin/env python3
"""Guard the PSP GE near-plane rendering contract.

The PSP hardware only clips the near plane when GU_CLIP_PLANES is enabled.
When it is disabled, geometry crossing the near plane or passing behind the
camera may be discarded as an entire primitive: giant walls and floor slabs
visibly vanish as the first-person viewpoint moves.

This is a source-configuration regression test. A real PPSSPP camera-walk
visual check is still required to validate hardware-specific guardband limits.
"""
from pathlib import Path

root=Path(__file__).resolve().parents[1]
psp=(root/"src/psp/PspRenderer3D.cpp").read_text(encoding="utf-8")
init=psp.split("PspRenderer3D::PspRenderer3D()",1)[1].split(
    "PspRenderer3D::~PspRenderer3D()",1)[0]
draw=psp.split("void PspRenderer3D::draw(",1)[1]

def verify(section: str, label: str, last: str) -> None:
    enable="sceGuEnable(GU_CLIP_PLANES);"
    if enable not in section:
        raise SystemExit(f"{label}: near-plane clipping missing")
    if "sceGuDisable(GU_CLIP_PLANES)" in section:
        raise SystemExit(f"{label}: near-plane clipping unexpectedly disabled")
    if section.index(enable)>section.index(last):
        raise SystemExit(f"{label}: clipping configured too late")

verify(init,"initial GE render state","sceGuFinish();")
verify(draw,"per-frame 3D render state","sceGumDrawArray(")

assert "sceGumPerspective(68.0f,480.0f/272.0f,0.25f,60.0f);" in draw
assert "sceGuDisable(GU_CULL_FACE);" in init
assert "GU_DEPTH_TEST" in draw
print("PSP GE rendering contract PASS: clipping ON before initial and every 3D draw")
print("Warning: real-world camera-path visual inspection is still required.")
