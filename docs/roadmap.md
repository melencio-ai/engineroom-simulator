# ENGINE WATCH — PSP Development Roadmap

## v0.1/v0.2 — completed 2D gameplay foundations

- Engine control room, machinery floor navigation and LO system UI.
- Mission 001 pump start and ECR verification.
- Mission 002 strainer restriction, standby changeover, isolation, pressure restoration, alarms.
- Deterministic headless system tests and PSP/PPSSPP builds.

## v0.3 — first real 3D environment (current)

- **Native PSP GU/GUM perspective** rendering. No SDL2 2D game renderer on PSP.
- Same 3D meshes and camera in desktop OpenGL preview.
- One walkable ECR and main-engine-room space connected by a physical doorway.
- Procedural main engine, two generator sets, local LO pumps, strainer, piping, grating, railings and overhead structures.
- Collision against bulkheads and main machinery footprints.
- Camera-facing/proximity interactions mapped into existing missions 001 and 002.
- Shared HUD font atlas, crosshair, status/panel overlays.
- 3D world tests, PSP EBOOT compilation, desktop preview compilation and PPSSPP boot/screenshot smoke.

**Review gate:** PPSSPP hardware/Android hands-on check for native 3D scene visibility, fonts, turning speed, walking controls and interactive station positioning before merging.

## v0.4 — turn blockout into recognizable engine-room spaces

- Better low-polygon meshes for engine block, turbocharger, gensets and auxiliary equipment.
- Texture atlas for labels, decks, bulkheads, and machinery nameplates.
- Improved camera sensitivity, doorway collision, first-person interaction highlighting.
- Measured PSP performance (polygon budget, draw calls and GPU cache behavior).
- Separate instrument panel/console views where readability on 480x272 requires them.

## v0.5 — operational machinery expansion

- Diesel-generator start/stop, electrical load-sharing and ECR mimic page.
- Cooling-water pumps, seawater and freshwater loops, heat exchanger and alarms.
- LO instrumentation mapped to physical pump/strainer locations.

## v0.6 — engine operations

- Compressed air system, compressors, receiver banks and start permissives.
- Main engine readiness states and representative startup sequence.
- Alarm logic, troubleshooting and mission scoring.

## v0.7+ — training and polish

- Multi-deck navigation, ladders and detailed machinery arrangements.
- Scenario selection, assessments, save/load, integrated help and maintenance tasks.
- Sound, equipment animations and more realistic process models once verified against an approved vessel reference.

## Simulation disclaimer

The figures, logic and operation steps are *illustrative educational game mechanics*, not manufacturer-approved or ship-specific operating instructions.
