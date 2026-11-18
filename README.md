# ENGINE WATCH 3D — Multi-deck PSP Engine Room Simulator

**v0.4 feature prototype** — Real-time, first-person 3D marine machinery space for PSP/PPSSPP and a matching desktop OpenGL preview. The engine room has **four principal working platforms (L1–L4)** plus a **lowest tank-top access level (L0)**, with an open vertical main-engine well.

The layout represents a *fictional composite merchant cargo ship*, not a specific vessel. Machinery behavior is illustrative gameplay logic and must not be used as actual shipboard operating instructions.

## What's new in v0.4?

- A single, **continuous five-height-band engine room** with working platforms separated vertically by 3.2 illustrative game-world metres.
- A six-cylinder main engine built **once** and extending from its tank-top foundation through upper platforms, visible through open machinery wells.
- Connected forward and aft crosswalks, port/starboard grating platforms, railings, overhead pipes, lighting, bulkheads, bilge access pockets and structural details.
- **L4:** ECR/control room with a doorway and overlooking windows.
- **L3:** two diesel generator models, auxiliaries, compressors and workshops.
- **L2:** main engine service catwalks, local access panels and exhaust/cooling lines.
- **L1:** main and standby LO pumps, valves, suction strainer and local control panel.
- **L0:** tank top, engine foundation, bilge wells and lower shaft access.
- Two separated ladders with deck shaft openings, **UP/DOWN selection** and continuous (animated) vertical camera movement.
- Deck-aware collision and interaction so controls on another floor cannot be used through the deck.
- PSP camera pitch control (L/R shoulders), so the player can look into the machinery well from above or examine overhead equipment.
- Fix to the PSP font texture UV coordinates: HUD text previously sampled transparent texels in `GU_TRANSFORM_2D`.
- Both LO system missions inherited from v0.2 remain connected to L1 and L4.

### Prototype limits

This is an optimized low-polygon **functional 3D blockout**, not a photorealistic ship or certified trainer. Generator/main-engine models are scenery (not operable yet); the currently simulated systems are LO pumps, valves, pressure, strainer fault and associated ECR actions. Ladder traversal is a short automatic camera climb after selecting a destination, not a hand-over-hand animation.

For the design, deck arrangement, source references and acceptance criteria, see **[Multi-deck engine room layout](docs/multideck-engine-room-layout.md)**.

## Controls

| PSP | Action |
| --- | --- |
| Analog stick | Walk forward/back and strafe |
| D-pad Left/Right | Rotate viewpoint |
| L trigger / R trigger | Look **down / up** |
| D-pad Up/Down | Walk when exploring; select rows inside panels |
| Cross (X) | Interact, open ladder panel, climb, operate pump/valve |
| Circle (O) | Close a panel |
| Triangle | Help |
| Start | Pause/resume; X while paused resets |

**Ladder:** Move near the yellow ladder landing; press X, select UP or DOWN, press X again, and the camera climbs to the adjacent platform. The protected-looking alternative ladder is a simplified visual aid and should not be taken as a regulation-compliant escape route model.

**Desktop native preview:** WASD to move, Left/Right or Q to turn, I/K or PageUp/PageDown to look up/down, X/E/Space to interact, C/Esc to close, H/F1 help, P/Tab pause. Right gamepad stick supports look controls.

## Mission walkthrough

1. **Mission 001:** Start in **L4 ECR**. Walk through the centre doorway and descend three decks on the service/escape ladder to **L1 Pump Flat**. Locate the local LO panel, start the main pump, then climb back to **L4** and verify LO header pressure at the ECR console.
2. **Mission 002:** After progressing at the ECR, return to **L1**, diagnose the strainer restriction, open standby valves, start standby, secure main, and return to L4 to verify pressure recovery.

During either mission you can explore L0–L4. The process model remains unchanged from v0.2; only physical navigation and presentation are reworked.

## Windows PPSSPP: download and play

1. Open the [GitHub Actions build runs](https://github.com/melencio-ai/engineroom-simulator/actions/workflows/build.yml) for the `feature/psp-v0.4-multideck-environment` branch.
2. Select a successful run of **Engine Watch 3D Multi-Deck CI**.
3. Download the `engine-watch-psp-v0.4-multideck` **artifact ZIP** in the run's Artifacts section (GitHub sign-in may be needed).
4. Extract it. The internal layout is `PSP/GAME/ENGINEWATCH/EBOOT.PBP`.
5. In PPSSPP, choose **File > Open Memory Stick Folder** (or **Settings > System > PSP Memory Stick**). Copy the whole `ENGINEWATCH` folder into that memory stick's `PSP/GAME/` directory.
6. Start the game from **Homebrew & Demos**. Always launch a fresh session instead of loading an old v0.3 savestate.

Do not run `EBOOT.PBP` from an arbitrary ZIP path if PPSSPP cannot resolve `ms0:/PSP/GAME/...`; install it into the configured memory stick first.

## Native PSP build

Use [PSPDEV](https://pspdev.github.io/):

```bash
psp-cmake -S . -B build/psp -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF
cmake --build build/psp -j2
```

Output: `build/psp/EBOOT.PBP`.

## Desktop preview and headless tests

SDL2 and OpenGL headers are required for the preview:

```bash
cmake -S . -B build/desktop -DENGINEWATCH_BUILD_GAME=ON
cmake --build build/desktop -j2
ctest --test-dir build/desktop --output-on-failure
./build/desktop/engine_watch
```

For portable geometry/simulation tests without SDL2/OpenGL:

```bash
cmake -S . -B build/tests -DENGINEWATCH_BUILD_GAME=OFF -DBUILD_TESTING=ON
cmake --build build/tests -j2
ctest --test-dir build/tests --output-on-failure
```

## Source map

- `src/core/EngineSimulation.*`: original process model and LO missions.
- `src/scene/World3D.*`: all decks, engine/pumps/gensets, ladders, geometry, deck-scoped collision.
- `src/scene/Game3D.*`: camera position/deck/pitch, ladder traversal and machinery interactions.
- `src/scene/FontAtlas.*`: 5x7 HUD glyphs.
- `src/psp/*`: native GU/GUM 3D pipeline and controller inputs.
- `src/desktop/*`: desktop SDL2/OpenGL camera and mesh preview.
- `src/ui/*`: retained unused v0.2 2D implementation.
- `tests/*`: 3D floor geometry/traversal and LO system tests.
- `docs/qa-multideck.md`: manual test route for PPSSPP Windows.

**Review gate:** develop on a separate branch, validate on PPSSPP and **do not merge** to other branches without approval.
