# ENGINE WATCH - Multi-deck Merchant Ship Engine Room Layout (v0.4)

Status: **approved design direction / PSP prototype implementation**, dated 2026-11-18.
Vessel reference: **fictional medium/large single-screw merchant cargo ship, low-speed diesel prime mover**.
This is an illustrative composite arrangement, **not a reproduction of a specific vessel, an approved General Arrangement drawing, or operational advice**.

## 1. Research and realism baseline

SOLAS II-2/13 machinery space access and its IACS interpretation recognize intermediate machinery decks, working platforms, passageways, the lower part of a machinery space and arrangements for emergency escape. They do **not** prescribe exactly four floors for all cargo ships. The precise machinery arrangement is vessel-specific and depends on propulsion machinery, hull and ship type. References:

- IACS UI SC277 Rev.1 (2024), *Escape from machinery spaces on cargo ships*, pp. 1-2, https://www.classnk.com/hp/pdf/info_service/iacs_ur_and_ui/ui_sc277_rev.1_may_2024_cln.pdf
- Lloyd's Register, 06/2025 *Engine Room Means of Escape Arrangements*, https://www.lr.org/en/knowledge/class-news/06-25/
- SOLAS II-2/13 and MSC.1/Circ.1511/Rev.1; IMO MSC.1/Circ.1689/Rev.1 (2026) update on escape from lower part of machinery spaces.

**Terminology clarification:** four **principal working platforms** are Level 1 through Level 4; Level 0 is an **additional, restricted/limited tank-top access zone**. The simulator therefore has *five height bands*, not four identical complete decks. Open machinery wells, partial flats and crossovers are preferable to stacking five sealed boxes.

## 2. Vertical section and equipment distribution

Illustrative heights in game-world metres, not dimensions from an actual vessel:

| Deck / height | Core layout | Machinery and interactions | Access |
| --- | --- | --- | --- |
| **L4: ECR / upper platform** y=12.8 | ECR at forward end, overview window; upper engine walkways and casing | LO mimic, monitoring consoles, alarm annunciators, access signage | ECR door, port main ladder, starboard escape ladder |
| **L3: auxiliary / generator platform** y=9.6 | port/stbd side flats, fore/aft crossovers, open engine well | two diesel generators, compressors/air bottles, workshop/purifier bays represented as placeholder modules | main and alternate ladder landings |
| **L2: main-engine operating platform** y=6.4 | port/stbd catwalks around engine; open sight lines | cylinder maintenance access, local service instruments, exhaust/manifold visual details | main and alternate ladder landings |
| **L1: pump flat** y=3.2 | lower port/stbd walkways, engine well, aft/forward crossovers | main and standby LO pumps, local LO control panel, LO suction strainer, cooling/FO pump placeholders | main ladder and alternate ladder |
| **L0: tank top / limited bilge access** y=0.0 | tank top walkway with engine foundation and shaft direction | bilge wells, drains, lower piping, main-engine bedplate | ladders, selected access pockets |

The main engine is built as **one vertical 3D machine** spanning the tank top through the upper platforms; it must not be duplicated as a different engine on each deck.

## 3. Horizontal plan, orientation and circulation

- **X axis:** forward (-X, ECR) to aft (+X, shaft tunnel). **Z axis:** port (-Z) and starboard (+Z). Y is vertical.
- Physical envelope: roughly X -10 to +15.5, Z -7 to +7. Hull shell, framing, deck beams and ventilation/pipe racks shape the space.
- **Center:** continuous main-engine void surrounded by port/stbd partial-grating platforms. Ensure openings through the flats so the engine can be seen vertically.
- **Forward end:** ECR on the upper level; generator and auxiliary bays on L3; pump manifolds and local LO stations on L1.
- **Aft end:** engine flywheel/shaft direction, maintenance space and alternate vertical escape connection.
- **Port main ladder:** near X -0.8, Z +5.0 (starboard in this coordinate convention); all five levels. Can be reached from the forward crosswalk.
- **Alternate escape ladder:** near X +13.5, Z -5.0. Physically separated from the primary ladder, all levels. Render recognizable protected-trunk casing and self-closing-door *visual cues*, without claiming that a simplified game model meets SOLAS construction criteria.
- Directional labels: DECK 04 - ECR, 03 - AUXILIARIES, 02 - MAIN ENGINE, 01 - PUMP FLAT, 00 - TANK TOP.
- Keep main passage clear around ladder landings. Include crossovers near the forward and aft boundaries of the central engine void.
- Keep railings at exposed edges, continuous safety-colour coding and safe sight lines from the ECR window.

## 4. Player movement and mission remapping

- First-person traversal must preserve the player position in **3D world coordinates**, including Y.
- Each deck has its own **walkable platform footprints, solid equipment obstacles and ladder opening exclusions**; a generic 2D rectangle is insufficient.
- The two main ladder locations are approach-and-press-X interactions. A ladder panel offers **UP / DOWN** based on the current deck.
- On selection, move the player through the shaft with **continuous vertical movement/transition** before placement at the next landing, instead of switching scenes.
- Allow Level 0 access for exploration and future missions; initial live machines remain on L1 and ECR on L4.
- **Mission 001**: start L4 ECR -> descend to L1 -> use local main LO pump -> ascend to L4 -> verify LO header at ECR.
- **Mission 002**: start from ECR -> descend to L1 -> investigate main suction strainer restriction -> open standby valves/start standby pump/secure main -> return to L4 -> verify.
- Existing LO hydraulic/alarm core in `src/core/EngineSimulation.*` remains the source of truth. Do not hard-code pressure just for 3D displays.

## 5. Asset placement / blockout palette

- Main engine: shared bedplate and crankcase low, 6 cylinder modules, exhaust manifold, upper scavenge/turbo visual assembly, flywheel, service platform.
- Gensets (2): visually distinct modular diesel blocks, alternator drums, local status boards, ventilation and exhaust.
- Pump flat: two identifiable LO pumps, discharge lines, valve wheels, strainer, service console with usable reach and clear cross-passage.
- Tank top: service pipe racks and recessed bilge-well markers, never depict unsafe walkable uncovered pits.
- Upper level: ECR monitors, window toward the machinery well, emergency signage, suspended lighting, overhead pipes.
- Consistent materials: blue-grey machinery, dark deck/steel grating, bright yellow guardrails, red exhaust/heat, cyan cooling/service water, warm work lights, green operational indicators. Colour coding is **illustrative** (not necessarily an approved piping standard for any vessel).

## 6. PSP technical strategy

- Target the native PSP 480x272 viewport and GU/GUM, with desktop OpenGL preview.
- Create low-poly, **shared scene geometry** and flat shaded surfaces; avoid large external textures, excessive draw calls and high vertex counts.
- Use short-repeating deck strips/rail modules with real open central wells, not five solid full-width floors.
- Use adjacency/level-based scene visibility and distance culling as needed after performance measurements; avoid hiding the multi-deck engine unrealistically.
- Reuse the shared ECR/pump/strainer mission interface and 5x7 HUD. Ensure overlays remain legible in PPSSPP.
- Implement deterministic headless tests covering: deck heights, shaft openings, walls, engine-core collisions, ladder links L0-L4, mission preservation, and level-specific interaction range.
- Run CI: headless C++ tests, desktop OpenGL compile, PSP EBOOT build, PPSSPP smoke boot/screenshots.

## 7. Completion / acceptance criteria

1. Five distinguishable height bands (four major platforms + lowest tank-top access) exist in one continuous world.
2. ECR visibly overlooks machinery from the upper level; main engine is continuous across multiple floors.
3. Player can visit every level using at least one continuous vertical traversal route, with an alternate route.
4. Ladder input cannot accidentally send the player outside accessible decks or trigger equipment on a different level.
5. Solid bulkheads, machinery and deck openings prevent inadvertent walking into voids.
6. Existing LO missions 001 and 002 remain operable between Level 4 and Level 1.
7. PSP EBOOT compiles; native PPSSPP rendering is manually inspected before merge.
8. Do not merge this feature branch to `feature/psp-v0.3-native-3d`, `develop` or `main` without explicit approval.

## 8. Next iterations (outside this first blockout)

- Real inclined ladder animations/handholds, accessible stair flights, camera pitch controls.
- Label textures and legible room/pipe system nameplates.
- Generator operation, purifier system, starting air, cooling circuits, engine local control stand.
- Advanced hull silhouette, structural detail, doors/escape compliance modelling, level-sensitive sound.
- Vessel-specific reference GA/approved documentation before any claims of exact proportions or operational training fidelity.
