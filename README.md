# ENGINE WATCH 3D — PSP Marine Engine Room Simulator

**v0.3 prototype: real-time, first-person 3D.** The v0.2 schematic/top-down renderer has been replaced by 3D geometry and a walkable engine room. Existing C++ LO system simulation and missions remain intact.

## 3D environment

- Connected engine control room (ECR) and machinery deck, with a passable doorway and solid collision geometry.
- Low-polygon six-cylinder main diesel engine, two diesel-generator sets, LO pump assemblies, main suction strainer, ECR console, deck grating, piping, handrails and overhead beams.
- Real perspective/depth-tested rendering through **PSP GU/GUM**; same procedural mesh, camera and interaction state are used in the optional desktop OpenGL preview.
- Face nearby equipment and press X to use the LO pump station, inspect the strainer or open the ECR console.

This is a **3D blockout**, not a photorealistic environment. Main engine and generator models are currently visual only. Textures, animation, additional operating systems and collision refinement are future work.

## Playable missions

**001 — Start and verify LO pump:** walk to the local station, start the main LO pump, allow pressure to stabilize and verify it back at the ECR.

**002 — Strainer restriction:** inspect high suction strainer DP, open standby valves, start standby pump, stop/isolate main pump, and verify alarm/pressure recovery at the ECR.

These are illustrative gameplay models. **Never use the values or steps as actual vessel operating instructions.**

## Controls

| PSP input | Action |
|---|---|
| Analog stick | Walk forward/back and strafe |
| D-pad Left/Right or L/R | Turn camera |
| D-pad Up/Down | Walk, or navigate menu rows when a panel is open |
| Cross (X) | Interact / operate |
| Circle (O) | Close panel |
| Triangle | Help |
| Start | Pause / resume (Cross while paused resets) |

Desktop preview: WASD movement, Left/Right or Q turning, X/E/Space interaction, C/Esc back, H/F1 help, P/Tab pause, and Up/Down menu navigation. Game controllers are also supported.

## PSP build and installation

Use the maintained [PSPDEV SDK](https://pspdev.github.io/).

```bash
psp-cmake -S . -B build/psp -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF
cmake --build build/psp -j2
```

Copy `build/psp/EBOOT.PBP` into the PPSSPP/PSP memory stick directory `PSP/GAME/ENGINEWATCH/EBOOT.PBP`. A successful GitHub Actions PSP job also publishes an `engine-watch-psp-v0.3-native-3d` artifact.

## Desktop 3D preview / tests

Install SDL2 and OpenGL development headers.

```bash
cmake -S . -B build/desktop -DENGINEWATCH_BUILD_GAME=ON
cmake --build build/desktop -j2
ctest --test-dir build/desktop --output-on-failure
./build/desktop/engine_watch
```

Without graphics dependencies, run headless simulation and world tests:

```bash
cmake -S . -B build/tests -DENGINEWATCH_BUILD_GAME=OFF -DBUILD_TESTING=ON
cmake --build build/tests -j2
ctest --test-dir build/tests --output-on-failure
```

## Source layout

- `src/core`: v0.2 pump, valve, pressure, strainer, alarm and mission simulation.
- `src/scene`: geometry, obstacle collision, stations, first-person controls, HUD and font.
- `src/psp`: dedicated native GU/GUM 3D renderer and PSP controller entrypoint.
- `src/desktop`: matching desktop OpenGL preview with SDL2 input.
- `src/ui`: retained v0.2 2D code, **not part of the v0.3 build**.
- `tests`: headless simulation and 3D navigation/collision checks.

**Branch workflow:** no merge to `develop` or `main` without approval. Review the isolated changes against `feature/psp-v0.2`.
