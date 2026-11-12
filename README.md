# ENGINE WATCH — Marine Engine Room Simulator (PSP)

**Version 0.1 prototype** — A PSP-first, single-player hybrid marine engine-room simulator with two connected areas: the Engine Control Room (ECR) and a walkable machinery platform.

The first playable task is intentionally narrow: **enter the engine room, start a local lubricating-oil pump, wait for pressure to build, return to the ECR, and verify the pressure and running indication.** Both locations share a single deterministic machinery simulation.

## Implemented in this prototype

- 480×272 SDL2 game, with built-in bitmap font, self-contained schematic room graphics, and PSP-style controller controls.
- Top-down walkable **ECR** with status mimic and door to engine-room platform.
- Top-down walkable **engine-room platform** with local lubricating-oil pump panel.
- Pump request, start delay, actual running state, power permissive, pressure ramp-up and decay.
- Low pressure alarm logic if a pump is requested but fails to establish pressure.
- ECR verification step, mission completion banner, pause, help, and restart (desktop).
- Pure C++11 simulation core, unit tests, native desktop build, and PSP cross-build workflow.

**No actual vessel procedures or manufacturer operating limits are claimed.** The numbers in the simulator are illustrative placeholders and must not be used for ship operation or credentialed training.

## Build for PSP / PPSSPP

Install [PSPDEV](https://pspdev.github.io/) with its SDL2 package. The PSPDEV Docker image is an alternative.

```bash
psp-cmake -S . -B build/psp -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF
cmake --build build/psp --parallel 2
```

The output is `build/psp/EBOOT.PBP`. Run it in PPSSPP or place it on a homebrew-capable PSP at:

```
ms0:/PSP/GAME/ENGINEWATCH/EBOOT.PBP
```

If you use Docker, the [PSPDEV Docker instructions](https://pspdev.github.io/installation/docker.html) show how to mount this repo and run the same commands inside `pspdev/pspdev:latest`.

## Build for desktop (optional, for faster iteration)

Requires CMake, a C++ compiler, and SDL2 development libraries:

```bash
cmake -S . -B build/desktop -DCMAKE_BUILD_TYPE=Debug
cmake --build build/desktop
./build/desktop/engine_watch
```

Run the headless simulation tests without SDL2:

```bash
cmake -S . -B build/tests -DENGINEWATCH_BUILD_GAME=OFF -DBUILD_TESTING=ON
cmake --build build/tests
ctest --test-dir build/tests --output-on-failure
```

## Controls

| Action | PSP | Desktop |
|---|---|---|
| Walk | D-pad or analog | Arrows / WASD |
| Inspect / operate | Cross (X) | X / E / Space |
| Back / close | Circle (O) | C / Escape |
| Help | Triangle | H / T |
| Pause / resume | Start | P / Tab |
| Reset from pause | — | R |

When you approach a door, press **X** to switch areas. Approach the pump on the engine-room platform and press **X** to open its local panel, then **X** to request a start. After pressure climbs above 3.0 bar (illustrative), return to the ECR, approach the monitoring console, and press **X** to verify the system and complete Mission 001.

## Architecture

- `src/core`: portable, headless machinery logic. No SDL dependencies.
- `src/ui`: SDL2 renderer, input bindings, movement, machinery panels, and bitmap font.
- `src/main.cpp`: PSP module initialization and SDL startup.
- `tests`: deterministic simulation tests (start delay, pressure response, fault conditions, mission rules, timestep limits).
- `.github/workflows`: host tests and PSP EBOOT compilation with uploaded CI artifact.
- `docs`: engineering assumptions, milestones, and acceptance checklist.

## Limitations and next work

- Scene art is schematic top-down 2D, not yet pre-rendered 2.5D.
- Only one piece of controllable machinery is modeled (LO pump). The main engine is not yet startable.
- There is no audio, collision with fixtures, saved game, or user-facing fault injection yet.
- A PSP binary must be compiled with PSPDEV; desktop tests alone do not prove hardware performance.

The prototype prioritizes the **shared simulation state and gameplay loop** so later systems can plug into the same architecture without rewriting either room.
