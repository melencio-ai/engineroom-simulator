# ENGINE WATCH 3D — PSP Marine Engine Room Simulator

**Version 0.5 feature build: marine-green steel decks and physically walkable inclined stairs.**

A first-person PSP/PPSSPP engineering-room game in C++, with native Sony PSP GU/GUM 3D and optional desktop SDL2/OpenGL preview. Its world is a fictional single-screw merchant-cargo-ship composite; engineering dimensions, paint layouts and machinery are illustrative rather than drawings of any particular ship.

## What has been added in v0.5?

- **Marine green steel decking:** epoxy-look painted checker/diamond plate, raised low-poly diagonal tread bars, panel seams and grating near the central engine well.
- **Real ship structure:** warm off-white bulkheads, grey-green stiffeners and welded-looking brackets, painted safety handrails, structural deck beams, machinery foundations, bolts and brighter luminaire fixtures.
- **Physically walkable inclined stair flights:** two separated 50-degree steel stair routes spanning adjacent floors, with 16 anti-slip horizontal treads per rise, steel stringers, paired handrails, underside shields and actual landings.
- **Continuous vertical gameplay:** walking up/down the slope changes camera Y in real space. No ladder menu or Cross-to-teleport mechanic. There are guarded open stairwell cutouts and clear connecting corridors at each level.
- **More credible piping:** neutral/insulated pipe surfaces, short service-colour identification collars, flanges and pressure-gauge props rather than entire bright red/blue pipes.
- **Improved PSP rendering:** dense world triangles submitted in bounded batches to avoid one excessively large PSP GE draw call.
- **Existing LO simulation preserved:** mission 001 (main LO pump/pressure verification), mission 002 (suction strainer restriction/standby changeover/return to ECR) and local station interaction remain.

## World layout

| Floor | Space |
| --- | --- |
| 04 | ECR, control consoles, upper viewing area |
| 03 | Generator/auxiliary machinery flat |
| 02 | Main engine service platform |
| 01 | Pump flat with main + standby LO pumps, strainer, valves |
| 00 | Tank top, main engine bedplate, bilge covers and shaft access |

One six-cylinder slow-speed main engine continues through the open center of all platforms. The levels are partial flats and crosswalks, not sealed rectangular rooms.

### On-foot stair traversal

The **forward starboard stair** is near X -1.82 to 0.86, Z +5.0. From upper levels, walk toward the upper end near X -1.82 and descend in the positive X direction. To reach the next flight, go around the open stairwell using the parallel bypass at Z +3.94.

The **aft alternate stair** is near X +13.50, Z -4.54 to -1.88. It runs from the negative-Z side toward the centerline as it descends.

No interact key is required to use either stair: **move along its sloped tread line**. Looking up/down is controlled independently.

## Controls

| PSP input | Action |
| --- | --- |
| Analog stick | Walk and strafe (also walk on stairs) |
| D-pad Left/Right | Turn camera |
| D-pad Up/Down | Walk when exploring, select rows when a panel is open |
| L/R triggers | Look down/up |
| Cross (X) | Use machinery/control station |
| Circle (O) | Close panel |
| Triangle | Help |
| Start | Pause/resume; X while paused resets |

Desktop preview: WASD walk; left/right/Q turn; I/K or PageUp/PageDown look; X/E/Space use; C/Esc back; H/F1 help; P/Tab pause. Game controller support is included.

## Windows PPSSPP installation

No GitHub CLI is necessary:

1. Visit the [Engine Watch GitHub Actions builds](https://github.com/melencio-ai/engineroom-simulator/actions/workflows/build.yml) and select a successful run of **Engine Watch 3D Marine Materials CI** on branch `feature/psp-v0.5-marine-materials-stairs`.
2. Download artifact **`engine-watch-psp-v0.5-marine-stairs`** from the Artifacts section and save to `D:\Downloads`.
3. Extract its ZIP. Find the internal `PSP/GAME/ENGINEWATCH/EBOOT.PBP` file.
4. Close PPSSPP, back up your old EBOOT, and copy the new file over `C:\Users\Melencio\Documents\PPSSPP\PSP\GAME\ENGINEWATCH\EBOOT.PBP`.
5. Launch PPSSPP and select **ENGINE WATCH** under Homebrew & Demos. Do **not** load an old v0.3/v0.4 savestate.

The game can also be compiled with [PSPDEV](https://pspdev.github.io/):

```bash
psp-cmake -S . -B build/psp -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF
cmake --build build/psp --parallel 2
```

Output: `build/psp/EBOOT.PBP`.

## Tests and desktop preview

```bash
cmake -S . -B build/tests -DENGINEWATCH_BUILD_GAME=OFF -DBUILD_TESTING=ON
cmake --build build/tests --parallel 2
ctest --test-dir build/tests --output-on-failure

# Optional SDL2 + OpenGL preview:
cmake -S . -B build/desktop -DENGINEWATCH_BUILD_GAME=ON -DBUILD_TESTING=ON
cmake --build build/desktop --parallel 2
./build/desktop/engine_watch
```

See [materials and stair engineering plan](docs/v0.5-marine-materials-and-stair-geometry.md), [multi-deck layout](docs/multideck-engine-room-layout.md), [roadmap](docs/roadmap.md), and [Windows PPSSPP QA](docs/qa-v0.5-windows.md).

## Scope and safety

This is a **PSP-optimized low-poly simulation**, not photoreal rendering or an approved maritime training program. The stair geometry and graphic materials are illustrative, not certified for escape-route or shipbuilding compliance; the pipe IDs and machinery logic must not be used as shipboard operating instructions.

**Branch policy:** keep updates isolated and review/verify PPSSPP before merging into any prior branch, `develop`, or `main`.
