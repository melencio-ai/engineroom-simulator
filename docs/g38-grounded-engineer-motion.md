# G3.8 — Grounded Engineer Motion on Real Steel Stairs

**Parent:** G3.7.1 Quaternius humanoid quality, stacked above the G3.6 physical control sockets. **This branch:** `feature/g3-8-grounded-engineer-motion`. Preview-only; no parent branch or `main` merge is authorized.

## Why this pass exists

The imported Worker uses genuine baked **Idle, Walk and Interact** source clips, but until G3.8 its pace on inclined stairs was based only on planar distance and its two boots inherited the same single character-floor height. That makes motion too slow on an ascent/descent and makes one boot float or intersect a tread as the engineer walks down the slanted stairs. It also leaves the upper body vertically rigid on climbs.

## Changes

- **Travel-corrected gait:** Drive clip phase and motion from actual 3D distance (X, Y and Z); normalize motion for the slower 1.85 m/s 3D stair pace instead of 2.55 m/s deck speed. The world movement cap remains 1.60 m/s horizontally on stair flights. If collision blocks movement, the gait stops advancing.
- **Real surface probes:** While on stairs, test foot placements about 12 cm left/right of the character axis, offset by the alternating walking stride, through the existing `World3D::walkSurface` with a 5 cm support probe. The *same* physical deck and flight heights used to move the engineer determine individual foot elevations. Clamp each relative boot support to +/-20 cm so a wrong floor cannot teleport a leg.
- **Visual correction shared by both bodies:** Apply filtered foot offsets to each lower leg, falling to zero by the hip. Lean the upper body up to 6.5 cm in the direction of the measured rise or descent; leave hips and feet on the collision-authoritative support. No runtime skinning, new textures, permanent world vertices or larger `ENGINEER.EWM` sidecar are needed.
- **Stable transitions:** Blend the slope, individual foot contacts and stair entry/exit over several frames; preserve source clip topology. No rest/walk pose changes on flat deck.
- **Regression coverage:** New `engineer_real_stair_gait_and_foot_contact` CTest checks descent, measured foot support, unchanged mesh size and finite vertices, and actual ECR-to-Deck-03 stair navigation. All earlier ECR, machinery socket, Quaternius and floor tests remain in CI.
- **PSP packaging:** Use a separate G3.8 CI artifact and backed-up installer, including **both** `EBOOT.PBP` and `ENGINEER.EWM`.

## Strict boundaries

This is a **ground-contact improvement**, not a full rigged IK system. The imported Worker still has source Idle/Walk/Interact poses, but there is no dedicated source ladder animation. A physically targeted 3P button press still falls back to the pre-existing procedural right-arm IK until G3.9 bone-level contact retargeting is validated. The low-poly model and PPE are visual aids, not safety-approved equipment. No collision, deck geometry, machinery logic or station semantics were altered.

## Acceptance and field QA

1. GitHub Actions: native C++11 tests, scene/geometry budgets, Blender CC0 source checksum, EWM humanoid validation, PSP build, PPSSPP boot smoke.
2. Download the artifact **`engine-watch-psp-g38-grounded-engineer-preview`** from this branch's successful Actions run into `D:\Downloads`.
3. Exit PPSSPP. Install the new EBOOT and character sidecar from `PSP/GAME/ENGINEWATCH/` together.
4. Select third-person view (PSP **SELECT**, desktop **V**). Walk the ECR bridge, descend all four inclined stair flights, pause on landings and turn 180 degrees.
5. Check both boots remain near the real steel treads, no rapid vertical pops or excessive lean, no clipped bulkheads/railings, the Worker's orange coveralls and yellow helmet remain intact, and the Deck 01 LO pump station still operates correctly.
6. Record moving-camera video for manual approval. CI cannot certify the visible boot placement or animation quality on a real PSP/phone.

### Windows PPSSPP installation

```powershell
$downloads = "D:\Downloads"
$installer = Join-Path $downloads "install-g38-ppsspp.ps1"
Invoke-WebRequest -Uri "https://raw.githubusercontent.com/melencio-ai/engineroom-simulator/feature/g3-8-grounded-engineer-motion/tools/install-g38-ppsspp.ps1" -OutFile $installer
powershell.exe -NoProfile -ExecutionPolicy Bypass -File $installer -Downloads $downloads
```

On Android PPSSPP, copy both `EBOOT.PBP` and `ENGINEER.EWM` into the phone's active `PSP/GAME/ENGINEWATCH/` Memory Stick directory.

**Do not merge any stacked branch, including this one, without explicit approval.**
