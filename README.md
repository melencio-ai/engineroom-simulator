# ENGINE WATCH 3D — PSP Marine Engine Room Simulator

**v0.6 feature build: reusable parametric 3D shipbuilding components**

This is a first-person, multi-deck PSP/PPSSPP marine engine-room simulator in C++11, with a matching SDL2/OpenGL desktop preview. The environment is now built in part from an independent library of **33 reusable 3D marine components**, with **16 separate material/finish types**. Each component can be resized, rotated, placed in the engine room or exported as an individual **OBJ + MTL** asset for Blender and other 3D tools.

The game is a *fictional merchant cargo ship composite*. The geometry and process simulation are representative; it is not approved naval-architecture documentation or shipboard operating guidance.

## Reusable shipbuilding object library

The new source modules are:

- `src/scene/SceneGeometry.hpp` — shared compact PSP-friendly vertex/3D coordinate definitions.
- `src/scene/MarineParts.hpp` / `.cpp` — 33 independently generated component templates, 16 material finishes and transform/placement API.
- `src/scene/World3D.cpp` — the assembled five-height-band ship engine room built from individually recorded `MarinePartPlacement` instances and core engine geometry.
- `tests/MarinePartsTests.cpp` — checks every template, material, transform, and actual use in the world.
- `tools/export_marine_parts.cpp` — native OBJ/MTL exporter for independent 3D object reuse.

**[Complete object catalog and standards references](docs/marine-asset-catalog.md)**

### Component families

| Structural / architectural | Pipes / fittings | Equipment / hardware |
| --- | --- | --- |
| Steel plate and marine checker plate | Pipe spool, 90° elbow, tee, reducer | Gate, globe and butterfly valves |
| Open steel grating, stair tread | Weld-neck and blind flange | Y-strainer and pressure gauge |
| I-beam, channel, angle, bulb-flat, flat bar | Rubber gasket and pipe support | Hex bolt, hex nut, washer, U-bolt |
| Gussets, handrail posts/spans | Pipe clamp | Access cover, cable tray, worklight |

Colours are **surface finishes**, not physical steel grades. Actual ship-grade materials and approved dimensions must be sourced from class rules, GA drawings and component manufacturer catalogs.

### Using a part in the C++ environment

```cpp
#include "scene/MarineParts.hpp"

enginewatch::MarinePartLibrary catalog;
std::vector<enginewatch::Vertex3D> mesh;

// Place a 2-metre visual pipe spool with an insulated finish.
catalog.append(mesh,
    enginewatch::PipeSpool,
    enginewatch::PipeInsulation,
    enginewatch::PartTransform(4.0f, 5.0f, -5.0f,
                               2.0f, 1.0f, 1.0f));
```

The world also records each stamped part in `World3D::parts()` for future selection, editing, service tagging and inspection.

## Get 33 separate OBJ files

Open a **successful** v0.6 GitHub Actions run of [Engine Watch 3D Shipbuilding Components CI](https://github.com/melencio-ai/engineroom-simulator/actions/workflows/build.yml).

Under **Artifacts**, download `engine-watch-reusable-3d-objects-v0.6`. Its ZIP contains **33 separate `.obj` models and 33 companion `.mtl` files**, together with the object catalog. Each object is independent and can be imported into Blender or another mesh application.

Generate the same assets locally on Linux/macOS/Windows with CMake and a C++ compiler:

```bash
cmake -S . -B build/objects -DENGINEWATCH_BUILD_GAME=OFF -DBUILD_TESTING=OFF -DENGINEWATCH_EXPORT_ASSETS=ON
cmake --build build/objects --parallel 2
mkdir -p dist/marine-objects
./build/objects/enginewatch_export_parts dist/marine-objects
```

## The game environment

The engine room has **four main platforms L1–L4 plus an additional tank-top access level L0**, connected by two physically walkable inclined steel stair systems:

| Deck | Purpose |
| --- | --- |
| **04** | ECR and upper viewing gallery |
| **03** | Generator and auxiliary machinery |
| **02** | Main engine service platform |
| **01** | Main/standby lubricating-oil pump flat, local valves, suction strainer |
| **00** | Tank top, engine foundation and bilge access areas |

New reusable pieces enhance green marine checker plating, partial grating, structural steel profiles, handrails, lights, pipe skids, flanged valves, pipe identification bands, holding-down bolts and service gauges. The main engine remains a continuous six-cylinder low-poly mesh extending through several deck levels.

**Mission 001:** from Deck 04 ECR, walk down to L1, start the main LO pump, return to ECR and verify pressure. **Mission 002:** diagnose LO strainer restriction on L1, change over to standby pump and verify system recovery in ECR. Core pump simulation remains unchanged.

### PSP controls

- Analog: walk and strafe, including on inclined stairs.
- D-pad Left/Right: turn.
- L/R: look down/up.
- D-pad Up/Down: movement or menu selection.
- Cross (X): interact with machinery. **No Cross is required to climb the stairs.**
- Circle: close; Triangle: help; Start: pause/reset.

## Windows PPSSPP — install the new game

1. From a successful v0.6 CI run, download `engine-watch-psp-v0.6-modular-engine-room` into `D:\Downloads`.
2. Extract the ZIP. Inside it, find `PSP\GAME\ENGINEWATCH\EBOOT.PBP`.
3. Close PPSSPP and back up your current EBOOT.
4. Replace `C:\Users\Melencio\Documents\PPSSPP\PSP\GAME\ENGINEWATCH\EBOOT.PBP` with the extracted executable.
5. Open PPSSPP → **Homebrew & Demos** → **ENGINE WATCH**; use a fresh launch, not a savestate from v0.5.

**No GitHub CLI is needed** when downloading artifacts through the browser.

## Build, testing and limitations

The CI workflow validates headless LO simulation tests, world/collision/navigation tests, all 33 component templates, material lookup, transforms, desktop OpenGL compilation, native PSP EBOOT compilation, and a PPSSPP boot/framebuffer smoke check. CI also exports all OBJ and MTL assets automatically.

The system uses vertex-coloured **low-poly** meshes, not dense manufacturer CAD. Material metallic/roughness values are descriptive for future renderers and are not PSP physically based shaders. Flange drilling, pipe schedules, material grades, service pressures and structural adequacy **are not verified** against actual approved manufacturing dimensions. Do not use this game as a certified marine engineering or safety trainer.

For engineering and art direction see [multi-deck layout](docs/multideck-engine-room-layout.md), [marine material and stair plan](docs/v0.5-marine-materials-and-stair-geometry.md), and [reusable component catalog](docs/marine-asset-catalog.md).

**Merge gate:** work remains on a separate feature branch until actual Windows PPSSPP QA and explicit approval.
