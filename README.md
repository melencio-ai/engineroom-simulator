# ENGINE WATCH — Marine Engine Room Simulator

**Version 0.2 prototype** — An offline, native PSP / PPSSPP hybrid marine engine-room simulator. Walk between a control room (ECR) and machinery platform, operate local machinery, monitor hydraulic behavior, and practice elementary fault diagnosis.

## What's playable

| Scenario | Actions | Result |
|---|---|---|
| **Mission 001 — Start and verify LO pump** | Enter engine room, operate main LO pump from local panel, return to ECR, **explicitly verify** stabilized LO header pressure. | First mission passes; **X** in the ECR console begins Mission 002. |
| **Mission 002 — Suction strainer restriction** | Observe deteriorating pressure/strainer DP, inspect local strainer, open standby valves, start standby pump, stop and isolate main, verify at ECR. | Second mission passes only after diagnosis, pressure recovery and safe isolation. |

Mission 002 has main/standby LO pumps, four shutoff valves, strainer differential pressure, suction pressure, common LO header pressure, simplified oil temperature, delayed low-pressure alarm, and alarm acknowledgment. Parameters respond continuously to player actions. The model runs independently of the PSP UI and has headless C++ automated tests.

**All engineering values are illustrative for gameplay and are NOT operating instructions.** Real vessel machinery operation must follow the ship's approved procedures and manufacturer documentation.

## PPSSPP / Android installation

1. Extract a release ZIP into PPSSPP's configured **Memory Stick root** (configured in PPSSPP > Settings > System, depending on platform).
2. Check the full path *inside that memory stick folder*: `PSP/GAME/ENGINEWATCH/EBOOT.PBP`.
3. In PPSSPP, open **Homebrew & Demos**, select **ENGINE WATCH**, and perform a fresh launch (not an old emulator save state).
4. If the screen is blank, switch PPSSPP graphics backend from Vulkan to OpenGL, turn off **Skip buffer effects**/speedhacks, and restart. A temporary on-device diagnostic screen is displayed if SDL startup fails.

Game uses native 480×272 rendering; the font atlas is power-of-two sized for PSP graphics compatibility. The original black-screen issue was resolved and visibly tested in PPSSPP Linux. Android gameplay success was also confirmed by the user for v0.1.1. **New v0.2 gameplay still needs hands-on validation.**

## Controls

| PSP control | Game action |
|---|---|
| D-pad / analog stick | Move player; D-pad Up/Down selects a row while local pump panel is open |
| X (Cross) | Interact, operate selection, acknowledge alarm, verify mission, advance |
| O (Circle) | Close panel |
| Triangle | Open/close help |
| Start | Pause/resume (then X to reset the simulation) |

Desktop build uses arrows/WASD, X/E/Space/Enter for action, C/Esc/Backspace for back, H/T/F1 for help, P/Tab for pause, and R for reset while paused.

## Build (native PSP)

Use the maintained [PSPDEV toolchain](https://pspdev.github.io/), with SDL2 and CMake.

```bash
psp-cmake -S . -B build/psp -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF
cmake --build build/psp -j2
```

The output is `build/psp/EBOOT.PBP`. Put it in the PPSSPP memory stick folder described above.

## Build and test (desktop)

```bash
cmake -S . -B build/desktop -DENGINEWATCH_BUILD_GAME=ON
cmake --build build/desktop -j2
ctest --test-dir build/desktop --output-on-failure
```

If SDL2 development headers are not available, test the headless machinery simulation:

```bash
cmake -S . -B build/tests -DENGINEWATCH_BUILD_GAME=OFF -DBUILD_TESTING=ON
cmake --build build/tests -j2
ctest --test-dir build/tests --output-on-failure
```

## Code layout

- `src/core/EngineSimulation.{hpp,cpp}` — platform-independent system, pumps, valves, pressure, alarm and missions.
- `src/ui/GameApp.{hpp,cpp}` — PSP/desktop controls, connected scenes, machinery panels, and HUD.
- `src/ui/BitmapFont.{hpp,cpp}` — no external font resources.
- `tests/EngineSimulationTests.cpp` — six headless test suites covering mission flow, faults, valves, alarms and restoration.
- `docs/mission-001.md`, `docs/mission-002.md` — mission briefs.
- `.github/workflows/build.yml` — GitHub Actions PSP cross-build, simulation tests and optional PPSSPP boot screenshot.

**Workflow:** new features are developed on separate branches and submitted to draft PRs. Do not merge into `develop` or `main` without explicit approval.
