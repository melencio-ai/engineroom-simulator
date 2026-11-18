# Engine Watch 3D v0.5 — Windows PPSSPP Acceptance Test

Target: PSP artifact `engine-watch-psp-v0.5-marine-stairs` from **Engine Watch 3D Marine Materials CI**.

## Install on this Windows machine

- Actual downloaded ZIP directory: `D:\Downloads`.
- Actual game EBOOT location: `C:\Users\Melencio\Documents\PPSSPP\PSP\GAME\ENGINEWATCH\EBOOT.PBP`.
- GitHub CLI (`gh`) is not installed. Download ZIP in the browser and extract manually, **not** using a `gh` command.
- Close PPSSPP first; back up the old EBOOT to a separate filename. Copy the extracted `PSP/GAME/ENGINEWATCH/EBOOT.PBP` over the configured EBOOT.
- Restart PPSSPP and launch the game from **Homebrew & Demos**. Don't use old savestates.
- Artifacts expire; use a recent successful run.

## Expected visual results

- [ ] Deck 04 ECR spawn; off-white light bulkheads and a readable LO status HUD.
- [ ] Green-painted raised-pattern steel floor visually covers most general walkways.
- [ ] Checker/diamond pattern and panel seams are visible near the camera, without severe moiré.
- [ ] Partial green grating beside the main-engine well; handrails and toe boards.
- [ ] No solid blocking floor over the centerline engine void; the engine is one vertically continuous assembly.
- [ ] Two **inclined** stair systems, with visible horizontal anti-slip treads, stringers, handrails and safety nosings. No vertical ladder teleports.
- [ ] Structural supports, foundation bolts, local light fittings and restrained service ID bands on neutral/insulated pipe bodies.
- [ ] No dark/blank screen or missing HUD glyphs on native PSP GU renderer.

## Movement, geometry and gameplay

- [ ] D-pad Left/Right rotates, analog walks, L/R looks vertically.
- [ ] From the ECR walk through the central door to the **forward starboard service stair** near X -1.82, Z +5.
- [ ] Hold the direction of the inclined flight. Camera Y changes continuously as the player descends from L4 to L3. **Cross should not be necessary to climb.**
- [ ] At L3, go around the open stairwell using the forward bypass passage near Z +3.94, then descend to L2.
- [ ] Repeat down through **L1 pump flat** and **L0 tank top**.
- [ ] Reverse travel on the same physical stair; ascend to L4.
- [ ] Try alternate aft stair at X +13.50, Z -4.54 to -1.88.
- [ ] Check landing clearances: standby LO pump and auxiliary generators should not block stairs.
- [ ] Check physical collision: walls, engine foundation, pump stools, central engine void and stairwell guard edges.
- [ ] Level indicator changes correctly near actual landings.
- [ ] L1: operate main LO pump; pressure increases after delay. Return to L4 ECR and verify mission 001.
- [ ] Mission 002: inspect main suction strainer on L1, switch to standby LO as required by game and verify alarm clearance at L4.
- [ ] Pause and reset returns to L4 ECR; mission 001 initial state restored.

## Performance and debug notes

- [ ] PSP framebuffer shows nonblank interior in native CI smoke screenshot.
- [ ] Compare PPSSPP rendering at 1× and 2×; record FPS and stair camera jitter.
- [ ] If checker-plate detailing flickers or aliasing is distracting, reduce tread geometry density in next pass.
- [ ] Capture five screenshots: ECR, aux platform, engine platform, pump flat, tank top.
- [ ] If frame rate is low, prioritize mesh batching, culling/occlusion, and a small texture atlas over additional procedural facets.
- [ ] Log precise coordinates and screenshot for unreachable machinery or collision clipping.

## Gate

Native EBOOT compilation, successful emulator boot frame and headless unit tests do **not** replace visual/manual route QA. Leave the feature PR as a draft until Windows PPSSPP and real-device game-feel checks are approved.
