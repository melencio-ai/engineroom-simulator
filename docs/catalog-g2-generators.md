# Engine Watch — G2 parametric geometry and LOD export

**Branch:** `feature/catalog-g2-parametric-3d-generators`  
**Parent PR:** [G1 catalog foundation, PR #9](https://github.com/melencio-ai/engineroom-simulator/pull/9)  
**Design constraint:** Do not replace the PSP game's validated v0.7.1 deck meshes or change engine-room missions during the asset-generation gate.

## Delivered now

Six independent **parametric mesh generator families** produce **seven different variants** from the registered G1 records:

| Family | Illustrative registered variants | Detailed inspection | PSP output |
| --- | --- | --- | --- |
| Hex-head bolts | M10×30, M12×40 | Raised helical thread rib, smooth shank, bevelled hex head | Hex silhouette and faceted smooth shank |
| Hex nuts | M12 | Hex prism, hollow bore and helical internal thread relief | Hollow, low-sided hex ring |
| Plain washers | M12 | Solid annular steel geometry with real central clearance hole | Low-sided hollow ring |
| Pipe spools | DN50, 800-mm nominal sample | Open-ended pipe, separately visible bore and wall | 12-sided hollow pipe |
| Weld-neck flanges | DN50 illustrative sample | Tapered hollow hub, open bore and genuine through-bolt holes | Lower-sided flange with actual drilled holes |
| Ring gaskets | DN50 illustrative sample | Hollow fiber sheet annulus with thickness | Low-sided annulus |

**Engineering limitations:** Example nominal sizes and values are not approved dimensions; `DN50` by itself does not specify an OD/wall/schedule. Class ratings, gasket suitability, thread classes, bolt preload/strength, weld details and OEM approval are absent. The meshes are **illustrative polygonal training assets, not solid CAD or manufacturing drawings**. Helical ribs are a visual thread approximation, not guaranteed ISO 68-1 thread profiles.

## Generate files

Requires Python 3.10+ and `jsonschema`:

```bash
python -m pip install "jsonschema>=4.20,<5"
python tools/asset_pipeline/validate_catalog.py
python -m unittest discover -s tests/catalog -p 'test_*.py' -v
python tools/asset_pipeline/export_catalog.py --out dist/parametric-catalog
```

Exports one set of files **per catalog variant**:

```text
dist/parametric-catalog/
  manifest.json
  inspection/
    <component-id>.obj     # high-detail independent geometry; millimetres
    <component-id>.mtl     # illustrative visual material
    <component-id>.glb     # glTF 2.0 binary; inspection detail + normals
  psp/
    <component-id>.obj     # simpler LOD in millimetres for visual comparison
    <component-id>.mtl
    <component-id>.ewmesh  # game-friendly binary vertex stream in metres
```

The manifest records every catalog ID, source JSON SHA-256, source/approval status, geometry bounds, triangle counts, separate output hashes, and the LOD's dimension unit.

**Native runtime-layout verification:** The CI compiles `tools/asset_pipeline/verify_ewmesh.cpp` as C++11 and reads every generated `.ewmesh` into `enginewatch::Vertex3D`. It enforces the `16-byte` ABGR/XYZ layout, version, triangle-multiple count, per-part PSP vertex cap, finite coordinates, alpha channel, and metre-scale bounds. This prevents silent mismatch between the Python asset exporter and the C++ scene format; gameplay import is still a separate later task.

### The PSP `.ewmesh` format (version 1)

```text
byte  0-3  "EW3D"
byte  4-7  uint32 little-endian format version = 1
byte  8-11 uint32 little-endian triangle vertex count (multiple of 3)
byte 12+   N * (uint32 ABGR8888 + float32 X + float32 Y + float32 Z)
```

Coordinate values in this runtime stream are **metres**, matching the existing `Vertex3D` member layout in `src/scene/SceneGeometry.hpp`. Source dimensions, high-detail OBJ and GLB are in **millimetres**. Future stages can embed these streams into PSP EBOOT or load them as assets; **the current PSP executable does not automatically import these generated files**.

The exporter is deterministic: the same validated records yield the same asset hashes. No external 3D geometry libraries or Blender installation are needed.

## Quality and dimensions

The catalog validator checks ID registration, units, required family parameters, source authority, pipe OD/wall/bore consistency, thread sizes and port compatibility. The G2 geometry tests separately check:

- Seven distinct exported variants across six families
- Inspection triangle count exceeds PSP triangle count
- PSP mesh budget maximum of **1,800 triangles per component**
- Real open central bores in the nut and washer
- Four physical through-bolt holes and a hollow central bore in the flange
- Bolt overall dimensions and explicit 3D helical surface detail
- Opposed pipe-end ports and compatible gasket/flange/thread interfaces
- GLB header/buffer metadata and `EW3D` binary byte layout
- Millimetre-to-metre scaling and reproducible SHA-256 manifest

These tests check internal consistency, **not** the licensed compliance of the illustrative values.

## What still needs to be done

**G2b:** Build actual nut/bolt/washer pairings and pipe-to-flange-to-gasket assemblies with selected size/pressure/material sources, show exploded component separation and port compatibility in the detailed inspection viewer. Add dimensional provenance and minimum clearances, and optionally generate STEP/B-rep master CAD through a separate native CAD toolchain.

**G3:** Replace the disappearing-floor geometry with one shared source for rendered walkable surfaces and collision, plus camera-path screenshot regressions. Do not assume asset detail alone fixes unreliable deck geometry.

**G4:** Construct complete main/standby LO pump sets from the approved inventory, including motor, shaft/coupling, seals, bearings, pipes, valves, starter panel and removable covers.

**Release gate:** all catalog, mesh/geometry, LO process, PSP EBOOT, desktop, exporter and PPSSPP smoke checks must pass. This branch is stacked on draft PR #9, and must **not merge directly into main** without approval.
