# Engine Watch — Reusable 3D Marine Shipbuilding Component Library (v0.6)

**Implementation:** `src/scene/MarineParts.hpp`, `src/scene/MarineParts.cpp`.  
**Portable asset exporter:** `tools/export_marine_parts.cpp`.  
**Engine-room placement and assembled instances:** `src/scene/World3D.cpp` / `World3D::parts()`.  
**Status:** first-pass low-polygon procedural source models. **Not fabrication drawings, pressure-rated parts, approved ship details, or manufacturer 3D CAD.**

## 1. Architecture: make parts, not one-off scenery

Every installed item is a distinct `MarinePartPlacement` record containing:

- `part`: a stable `MarinePartId` identifying a generic component prototype.
- `finish`: a `MarineMaterialId` selecting a shipboard coating or surface appearance.
- `transform`: world `Vec3` location, local dimensions (scale), and yaw/pitch/roll.

A `MarinePartLibrary` generates **one local-space triangular mesh per component class**, once, then stamps copies into the PSP scene. The placed object records survive in `World3D::parts()` for inspection, editing or future interaction.

There is **no claim of hardware GPU instancing**; the PSP native GU uses vertex-coloured triangles, baked from placements into a static world mesh and submitted in bounded draw batches. This is intentionally C++11 and PSPDEV-friendly.

**Material versus coating:** Steel grade (for example, hull grade A, AH36, DH36) identifies physical properties. Green epoxy is a coating, not a steel grade. A checker-plate mesh may be assigned green epoxy, galvanized appearance or bare steel independently. Corrosion, wear, reflectivity and structural strength are not physically simulated.

## 2. Implemented 33 reusable 3D archetypes

| Group | 3D component IDs |
| --- | --- |
| Shipboard flooring | `SteelPlate`, `CheckerPlate`, `OpenSteelGrating`, `AccessCover` |
| Hull/flat construction | `IBeam`, `ChannelSection`, `AngleSection`, `BulbFlat`, `FlatBar`, `GussetBracket` |
| Access and rails | `StairTread`, `HandrailPost`, `HandrailSpan` |
| Pipe geometry | `PipeSpool`, `PipeElbow90`, `PipeTee`, `ConcentricReducer` |
| Pipe interfaces | `WeldNeckFlange`, `BlindFlange`, `PipeGasket`, `PipeClamp`, `PipeSupport`, `Ubolt` |
| Fasteners | `HexBolt`, `HexNut`, `FlatWasher` |
| Fluid-system equipment | `GateValve`, `GlobeValve`, `ButterflyValve`, `YStrainer`, `PressureGauge` |
| Services | `CableTray`, `WorkLightFixture` |

All catalog components are present as instances in the procedural multi-deck engine room, installed near machinery foundations, main-engine wells, stairs, overhead trays, water/LO lines, pumps, or a representative piping skid.

Each piece is a proper 3D triangle mesh: for example, flange rings have thickness and representative hex bolt heads; valves have stems and handwheels; grating consists of open steel bars; elbows have curved pipe bodies rather than sharp rectangular boxes. The objects have simplified silhouettes, but not merely 2D sprites.

## 3. Sixteen PSP finishes

| Finish ID | Intended appearance |
| --- | --- |
| `GreenEpoxy` | Green painted checker plate / marine walking surfaces |
| `GreenGrating` | Green-painted steel grating |
| `OffWhiteCoatedSteel` | Warm-white protected bulkheads and handrails |
| `GalvanizedSteel` | Galvanized metal fasteners and support brackets |
| `StructuralSteelGrey` | Grey-green structural members |
| `MachineryBlue` | Marine machinery coated blue |
| `PipeInsulation` | Silver-gray insulated/clad pipe body |
| `CastIron` | Dark cast valve and strainer bodies |
| `StainlessSteel` | Stainless cable-tray/fixture finish |
| `BronzeInstrument` | Bronze/brass instrument bodies |
| `SafetyYellow` | Selected hazard hardware |
| `LubricatingOilOrange` | LO pipe identification band |
| `CoolingWaterBlue` | Cooling-water identification band |
| `FireSystemRed` | Fire-system identification band |
| `RubberGasket` | Black resilient flange gasket |
| `WorkLightWhite` | Light fixture / diffuser |

Each `MarineFinish` contains an ABGR8888 PSP colour plus future roughness/metalness metadata; the latter **does not yet activate a PBR shader**. Colour bands are currently conceptual markers. Actual shipboard service identification requires the vessel's piping schedule and the applicable ISO 14726 scheme, including secondary colours.

## 4. Standards and dimensional references

Source labels define *families and how to parameterize them*; **the current normalized prototype dimensions are NOT claimed to equal standard-specific catalog dimensions.**

- Hull and ship structural steels, including grades A/B/D/E and AH/DH/EH strength families: Lloyd's Register steel material rules, https://www.imorules.com/LRMAT_CH3_3.html
- Steel pipe nominal size, OD, schedule and wall thickness: ASME B36.10, https://www.asme.org/codes-standards/find-codes-standards/welded-and-seamless-wrought-steel-pipe
- Flange pressure classes and bolt/gasket relationships: ASME B16.5 (2025), https://www.asme.org/codes-standards/find-codes-standards/b16-5-pipe-flanges-flanged-fittings-nps-1-2-nps-24-metric-inch-standard/2025
- Factory-made elbows, tees and reducers: ASME B16.9 (2024), https://www.asme.org/codes-standards/find-codes-standards/b16-9-factory-made-wrought-buttwelding-fittings
- Shipboard piping content identification colours: ISO 14726:2008 (confirmed 2024), https://www.iso.org/standard/44744.html
- Cargo machinery-space escape provisions: IACS UI SC277 Rev.1 (May 2024), https://www.classnk.com/hp/pdf/info_service/iacs_ur_and_ui/ui_sc277_rev.1_may_2024_cln.pdf

To generate **dimensionally accurate** flange / pipe assemblies later, pair a size record (DN/NPS, schedule, class, bolt PCD, bore, gasket specification) with each instance. A visually plausible flange cannot, by itself, encode or prove pressure class, suitability, connection integrity or structural strength.

## 5. Export each 3D object for reuse in Blender or other engines

The build contains a native OBJ/MTL exporter, requiring **no GUI and no PSP hardware**.

```bash
cmake -S . -B build/objects -DENGINEWATCH_BUILD_GAME=OFF -DBUILD_TESTING=OFF -DENGINEWATCH_EXPORT_ASSETS=ON
cmake --build build/objects --parallel 2
mkdir -p dist/marine-objects
./build/objects/enginewatch_export_parts dist/marine-objects
```

Produces **33 separate `.obj` meshes and 33 paired `.mtl` finish files**. OBJ vertices include a widely supported RGB vertex-colour extension; software without that extension can use the companion material's diffuse colour.

Alternatively, GitHub Actions publishes a ready-made **`engine-watch-reusable-3d-objects-v0.6`** artifact containing all the objects. The PSP game is separately available as `engine-watch-psp-v0.6-modular-engine-room`.

## 6. Where the engine room now uses them

- Raised chequer-panel and grating meshes: in repeated port/starboard main-engine access galleries.
- Structural I-beams, channels, bulb flats, brackets: around hull-side frames, engine spaces and overhead beams.
- Reusable tread panels and rails: at the inclined stairways, ECR and engine-side platforms.
- Pipe spools, elbows, tee, reducer, weld-neck flange, blind flange and gasket: around a representative LO skid and cooling header.
- Gate/globe/butterfly valves, Y-strainer and gauge: in the LO/pump demonstration zone.
- Bolts, washers, nuts, U-bolts and supports: engine holding-down details, clamps and flanged pipe joints.
- Cable tray, removable deck access covers and work lights: across the machinery flats.

### Collision and simulation policy

Installed objects remain visual unless explicitly promoted into the game's station/collision registry. A high-detail bolt should not create a walking collision capsule. Pumps, engine, bulkheads, open wells and physical stairs retain separate collision/interaction logic. LO hydraulic process states continue to use `EngineSimulation` without being derived from 3D mesh positions.

## 7. Test and future roadmap

Automated tests check **all 33** mesh templates are nonempty triangles, **all 16** material recipes valid, scale/rotation transformations work, invalid transforms are rejected, and every archetype has a placed instance in the engine room. Existing PSP, desktop and PPSSPP boot checks remain.

**Next additions:** dimensioned shipyard fabrication drawings for selected vessel, DN/NPS presets with B36.10 wall thickness, ASME B16.5 flange dimensions/bolt-hole relationships, longer bend radii, valve manufacturer catalog models, actual engine OEM spare parts, bulkhead penetration sleeves, expansion joints, heat exchanger internals, fire doors, and standardized label/texture atlases.

Do not model or label something *ISO/ASME/class compliant* without verifying the specific dimensional data, selected class, ship and operating pressure. This project is a visual/educational game, not a certified engineering reference.
