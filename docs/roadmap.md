# ENGINE WATCH — PSP Development Roadmap

## v0.1/v0.2 — completed 2D gameplay foundations

- Engine control room, machinery floor navigation and LO system UI.
- Mission 001 pump start and ECR verification.
- Mission 002 strainer restriction, standby changeover, isolation, pressure restoration, alarms.
- Deterministic headless system tests and PSP/PPSSPP builds.

## v0.3 — first real 3D environment (completed prototype)

- **Native PSP GU/GUM perspective** rendering. No SDL2 2D game renderer on PSP.
- Same 3D meshes and camera in desktop OpenGL preview.
- One walkable ECR and main-engine-room space connected by a physical doorway.
- Procedural main engine, two generator sets, local LO pumps, strainer, piping, grating, railings and overhead structures.
- Collision against bulkheads and main machinery footprints.
- Camera-facing/proximity interactions mapped into existing missions 001 and 002.
- Shared HUD font atlas, crosshair, status/panel overlays.
- 3D world tests, PSP EBOOT compilation, desktop preview compilation and PPSSPP boot/screenshot smoke.

**Review gate:** PPSSPP hardware/Android hands-on check for native 3D scene visibility, fonts, turning speed, walking controls and interactive station positioning before merging.

## v0.4 — multi-deck environment (completed prototype)

- Four working platforms (L1-L4) plus lowest tank top (L0), open multi-level engine well.
- ECR upper, auxiliary generators, operating platforms, pump flat and lower bilge access.
- Preserved LO simulation with floor-aware interaction and two ladder route markers.
- PSP font atlas UV corrected, desktop/PSP 3D render paths and CI smoke.

## v0.5 — marine materials and inclined stairs (current feature branch)

- Marine-green epoxy appearance on steel tread plates with raised chequer patterns, panel seams and selected open-grating catwalks.
- Warm off-white bulkheads, pale steel safety railings, grey-green transverse frames, welded brackets, plate girder/casing details, brighter task lamps.
- Two approximately 50-degree, 1-metre-wide inclined steel stair routes with separate landings, step noses, stringers, dual rails and underside shielding.
- Full first-person slope traversal: **walk between decks without pressing X or teleporting**; collision uses continuous foot height and stair-opening exclusions.
- Shipboard-looking neutral/insulated piping with coloured service bands rather than saturated pipe bodies, bolted bedplates and gauge details.
- Repositioned selected equipment to keep the main stair and the aft escape stair approaches unobstructed.
- PSP GU mesh submitted in safe batches to handle a denser visual scene; 480x272 low-poly material language remains.
- New world/navigation tests cover repeated stair rise, angle, collision, real player movement and preserved ECR/LO interactions.
- Design references, caveats and acceptance checks: `docs/v0.5-marine-materials-and-stair-geometry.md`.

**Review gate:** verify Windows PPSSPP visual lighting, green deck readability, horizontal and vertical traversability, frame rate and both missions before any merge.

## v0.6 — operational machinery expansion

- Generator start/stop, electrical load-sharing and ECR mimic page.
- Cooling-water pumps, seawater/freshwater loops, heat exchanger and alarms.
- LO instrumentation and pipe-name labels mapped to physical pump/strainer positions.

## v0.7 — main-engine operations

- Compressed air system, compressors, receiver banks and start permissives.
- Main engine readiness states and representative startup sequence.
- Alarm logic, troubleshooting and mission scoring.

## v0.8+ — training and polish

- Expanded spatial detail, equipment animations, maintenance platforms and new mission routes.
- Scenario selection, assessments, save/load, integrated help and maintenance tasks.
- Sound, equipment animations and more realistic process models once verified against an approved vessel reference.

## Simulation disclaimer

The figures, logic and operation steps are *illustrative educational game mechanics*, not manufacturer-approved or ship-specific operating instructions.
