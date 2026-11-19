# G3.5.2 — Smart Third-Person Engine-Room Camera

**Branch:** `feature/g3-5-2-smart-third-person-camera`  
**Parent:** [G3.5.1 PR #14](https://github.com/melencio-ai/engineroom-simulator/pull/14), stacked above G3.5 PR #13, G3 PR #12 and G1–G2b.  
**Purpose:** fix real PPSSPP footage showing stair treads obstructing the engineer (~12 seconds), bulkhead/column occlusion (~22–23 seconds) and camera moving too close to the helmet (~35–36 seconds).

## How the camera now works

The prior camera checked `World3D::walkSurface()` along a flat trailing track. A camera can be on a valid floor yet still sit inside the middle of a bulkhead, column, stair tread or overhead beam. When obstructed, the old logic could shrink the camera boom toward the character's head and obscure the scene.

G3.5.2 uses a **separate 3D visibility model**:

1. **Shared G3 deck cells:** `World3D::cameraRayFraction()` checks the physical green steel decks, service cover and thin grating collision envelopes. It obtains their dimensions from the same `DeckPlan` that renders them.
2. **Authored camera-only blockers:** the actual drawn hull bulkheads, ECR columns, walls, console bases, main-engine blocks, LO pump cabinets, overhead beams and **each of the 128 load-bearing stair treads** are described by conservative 3D AABBs. Camera blockers are created alongside physical meshes, not added as a separate invisible building.
3. **Fast swept-sphere query:** one ray from the engineer's upper-eye reference point to a candidate camera position intersects inflated AABBs via a C++11 slab test. A cheap 3D broadphase rejects distant boxes before division. There are no per-triangle GPU collision queries, no heavy library and no permanent new scene vertices.
4. **12 alternate camera positions:** start with the familiar trailing shoulder, then try right/left shoulders, raised rear views, side-offset elevated views and a lateral ECR/column bypass. The candidate must retain a useful unobstructed distance; a far clear side camera scores above a very close default camera.
5. **Stable transitions:** scores prefer the previously selected side when similar (hysteresis). Camera position and aim are eased, but every interpolated position is tested against the real structure. If smoothing would cross a solid blocker, the camera immediately snaps to the already clear alternative.
6. **No automatic view-mode switch:** PSP SELECT still selects 1P or 3P. The third-person body stays visible on ladders. The HUD shows `VIEW 3P SHIFT` for a side/overhead alternate or `VIEW 3P CLOSE` when the scene genuinely constrains the camera nearer than 1.45 m.

The first-person glove model, boiler suit, walk/climb animation, physical machinery interaction behavior, G3 disappearing-floor fix, PSP `GU_CLIP_PLANES` fix and 127,950-vertex static ship remain intact.

## Performance and limitations

- All occluders are small AABBs or known physical deck slabs; queries do not allocate or read the framebuffer.
- 3D broadphase avoids detailed slab tests for structural objects nowhere near the current camera ray.
- Twelve candidate positions are examined each update only in third-person. First-person camera gameplay does not perform the additional checks.
- The resulting visible viewpoint is an **approximation**, not a full photorealistic collision solution. Thin handrails, decorative piping and moving machine geometry may not have camera-specific occluders. An extreme narrow space may still require a close elevated view; visually test on PPSSPP.
- Physics/collision for **walking** remains unchanged. Never treat this system as actual shipboard navigation engineering clearance.

## Regression QA

`tests/PlayerCharacterTests.cpp` now explicitly checks that:

- A valid open ECR doorway remains clear.
- The ECR console and doorway jamb block a camera ray as expected.
- Hull walls, real solid deck cells and individual inclined steel treads interrupt occluded rays.
- Invalid query inputs are rejected without undefined values.
- In the ECR, a 3P camera is further than a helmet-close view and has a clear ray to the engineer.
- During in-place turns and an actual ECR-to-Deck-03 ladder walk, the full-body character stays rendered and camera sightlines do not cross blocked structures.
- PSP scene mesh and LO pump/mission behavior tests still pass.

The existing native CI still runs the five CTest suites, catalog geometry exports, desktop compilation, PSP EBOOT build and PPSSPP boot smoke. **Automated PPSSPP boot does not exercise the user's entire recorded camera path.** Manual moving-camera review is the final acceptance gate.

## Install on Windows PPSSPP

Download the new GitHub Actions artifact `engine-watch-psp-g352-smart-camera-preview` into `D:\Downloads` (GitHub login may be needed), close PPSSPP, and run:

```powershell
$downloads = "D:\Downloads"
$installer = Join-Path $downloads "install-g352-ppsspp.ps1"

Invoke-WebRequest -Uri "https://raw.githubusercontent.com/melencio-ai/engineroom-simulator/feature/g3-5-2-smart-third-person-camera/tools/install-g352-ppsspp.ps1" -OutFile $installer

powershell.exe -NoProfile -ExecutionPolicy Bypass -File $installer -Downloads $downloads
```

The installer backs up your previous EBOOT and does not require `gh`. On mobile PPSSPP, extract the same artifact and replace the game's `EBOOT.PBP` in the selected Memory Stick `PSP/GAME/ENGINEWATCH` directory.

## User visual sign-off

Recreate the positions near **12, 22–23 and 35–36 seconds** of the uploaded G3.5.1 gameplay recording:

- Stair: the third-person engineer should stay visible instead of being hidden by the underside of the steel treads.
- Columns: camera should move to the right/left or overhead before a white bulkhead occupies the entire foreground.
- Tight passage: the camera should retain useful distance from the helmet, with the crosshair aimed ahead.
- ECR bridge and Deck 01: no regression in walk/climb motion, wall/floor visibility, pump controls or `SELECT` camera toggle.
- Look for `VIEW 3P SHIFT` as the obstruction-aware camera chooses a better sightline. `VIEW 3P CLOSE` may still appear where no clear long shot exists.

**Release policy:** Keep [PR #15](https://github.com/melencio-ai/engineroom-simulator/pull/15) draft until PPSSPP/phone visual inspection and explicit approval. Do not merge any stacked dependencies into main without permission.
