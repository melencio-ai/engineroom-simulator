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

## v0.5 — marine materials and inclined stairs (completed prototype)

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

## v0.6 — reusable 3D shipbuilding components (current feature branch)

- Separate finish definitions from geometry types and placed instances.
- Generate **33 low-poly marine component meshes** (structural, deck, steel stairs, pipe systems, fittings, fasteners and fixtures) only once per type.
- Define **16 marine material finishes** independently, compatible with PSP vertex colours and future PBR metadata.
- Keep explicit, inspectable `MarinePartPlacement` instances with position/scale/rotation in the multi-level world.
- Use every archetype in actual engine-room assembly: catwalks, structure, pipework, machinery pipe skid, valves, bolts, instruments, lighting and supports.
- Remove duplicate checker-tread polygon generation to remain inside PSP geometry budget.
- Produce separate **OBJ + MTL assets** via a native exporter, shipped as a GitHub Actions artifact and usable outside PSP.
- Headless catalog coverage tests, existing world tests, desktop PSP build and PPSSPP smoke gates.
- Engineering reference and export instructions: `docs/marine-asset-catalog.md`.

**Review gate:** Verify the new EBOOT and exported assets on Windows. No branch merge without explicit approval.

## v0.7 — dimensioned marine systems

- Add explicit pipe NPS/DN/schedule lookup against ASME B36.10 and B16.5/B16.9 dimensional presets.
- Add local/remote operational valves, pressure-relevant fitting and connection relationships.
- Generator start/stop, electrical loading and ECR mimic.
- Cooling-water, seawater/freshwater loops and LO physical instrumentation.
- Reference vessel's approved machinery GA before definitive equipment placement.

## v0.8 — main-engine operations

- Starting air system, compressors and receivers.
- Main-engine readiness and safe permissive logic (illustrative).
- Alarm logic, troubleshooting and mission scoring.

## v0.9+ — training and polish

- Scenario selection, save/load, assessments and maintenance jobs.
- Sound, animations, vessel-specific access and inspection routes.
- Optional higher-detail desktop component assets, object editor and scene instancing.

## Simulation disclaimer

The figures, logic and operation steps are *illustrative educational game mechanics*, not manufacturer-approved or ship-specific operating instructions.
