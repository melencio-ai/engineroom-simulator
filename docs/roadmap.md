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

## v0.4 — multi-deck layout and regeneration (current feature branch)

- Four principal engine-room platforms (L1-L4) plus the lowest tank-top access (L0), in a single world.
- Multi-level main-engine mesh, open center well, four sets of partial catwalks with railings and grating.
- ECR upper overlooking windows, deck 03 twin diesel generators, deck 01 pumps/strainer, lowest bilge access.
- Two separated yellow ladder routes, floor openings and continuous vertical camera traverse.
- Deck-aware equipment interaction and obstacle collision; expanded vertical camera pitch inputs.
- Fixed PSP GU font atlas UV mapping to restore HUD readability.
- Headless floor/shaft/ladder tests, native PSP EBOOT and desktop OpenGL checks.
- New detailed source-backed layout document at `docs/multideck-engine-room-layout.md`.

**Review gate:** Manually inspect PPSSPP Windows/Android visuals, deck transitions, FPS, HUD visibility, and both LO missions. Do not merge without approval.

## v0.5 — machinery detail and interactivity

- Better low-poly engine cylinder heads, turbocharger, gensets and auxiliary equipment.
- Texture/label atlas for deck numbers, emergency paths and machinery nameplates.
- Proper inclined staircase or ladder motion with a readable climbing animation.
- Separate tooltips/console inspection views where 480x272 needs additional clarity.
- Performance profiling and platform/visibility culling on actual PSP/PPSSPP hardware.

## v0.5 — operational machinery expansion

- Diesel-generator start/stop, electrical load-sharing and ECR mimic page.
- Cooling-water pumps, seawater and freshwater loops, heat exchanger and alarms.
- LO instrumentation mapped to physical pump/strainer locations.

## v0.6 — engine operations

- Compressed air system, compressors, receiver banks and start permissives.
- Main engine readiness states and representative startup sequence.
- Alarm logic, troubleshooting and mission scoring.

## v0.7+ — training and polish

- Expanded spatial detail, equipment animations, maintenance platforms and new mission routes.
- Scenario selection, assessments, save/load, integrated help and maintenance tasks.
- Sound, equipment animations and more realistic process models once verified against an approved vessel reference.

## Simulation disclaimer

The figures, logic and operation steps are *illustrative educational game mechanics*, not manufacturer-approved or ship-specific operating instructions.
