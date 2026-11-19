# Engine Watch G2b — realistic component assemblies and exploded inspector

**Feature branch:** `feature/catalog-g2b-marine-assembly-builder`  
**Stacking order:** G1 [PR #9](https://github.com/melencio-ai/engineroom-simulator/pull/9) → G2 [PR #10](https://github.com/melencio-ai/engineroom-simulator/pull/10) → G2b [PR #11](https://github.com/melencio-ai/engineroom-simulator/pull/11)  
**Scope:** Source-driven educational 3D assemblies. Existing PSP game, Pump Flat, ECR and missions are unchanged.

## Completed assembly records

### 1. DN50 illustrative lubricating-oil flanged pipe joint

A single assembly ID, `ASM.LO.FLANGE.JOINT.DN50.ILLUSTRATIVE`, includes **21 independently identified physical pieces**:

| Part | Qty | Geometry and location |
|---|---:|---|
| Hollow DN50 pipe spool | 2 | 800 mm each; one on each side of the joint |
| Weld-neck flange | 2 | 4 drilled holes, bolt circle 125 mm (illustrative) |
| Fiber ring gasket | 1 | 3 mm between mating flange faces, central bore 52 mm |
| M16 × 60 bolt | 4 | 60 mm under-head length; illustrative helical inspection thread |
| M16 hex nut | 4 | 13 mm thick, threaded |
| M16 washer | 8 | Two per bolt at the flange's outer faces |

Each piece is generated from its own versioned part definition. Only **2 pipe/flange pairs are welded** and therefore marked non-removable. Bolts, nuts, washers and gasket are separately maintainable for the diagram.

Coordinate basis: millimetres, **X = joint/spool axis**, Y–Z = the drilling plane. Flange faces at X −1.5 and +1.5 mm sandwich a 3-mm gasket. The mating spool weld ends are at X −54.5 and +54.5 mm. Four fasteners are evenly arranged around a 125-mm bolt circle. Every washer seats on its matching flange rear face; the nut and bolt overlap fully within the modeled threaded length.

**Safety boundary:** all dimensions are explicitly illustrative, **NOT** sourced from approved ASME tables or OEM ratings. Bolt diameter, hole clearance, gasket design, flange pressure class, stud/nut strength, tightening torque, thread fit, weld geometry, corrosion and temperature compatibility are **not certified**. Do not use these assemblies to fabricate or pressure-test hardware.

### 2. Independent M16 four-part bolt kit

`ASM.FASTENER.M16.CLAMPKIT.ILLUSTRATIVE` contains one M16 bolt, two M16 washers, and one M16 nut. The visible free clamping gap intentionally does not represent a material stack or a correctly tightened fastener. The kit uses exactly the **same catalog variants** as the flanged joint.

### Deferred G1 placeholder

`ASM.LO.PUMP.SKID.DEMO.001` still references `MOTOR.LO.DEMO.TODO` and `PUMP.LO.DEMO.TODO`. G2b excludes this unfinished motor/pump assembly from generated mesh output, rather than inventing a motor or silently claiming an operational pump has been modeled.

## Data and assembly constraints

The `schemas/assembly.schema.json` contract now supports an optional structured `fit_spec` for flanged pipe joints. The primary `validate_catalog.py` pipeline invokes the new `assembly_builder.py` semantic checks, which reject:

- Missing/incorrect component or connection port IDs
- Misaligned flange, gasket or pipe ends; port normals not facing appropriately
- Different pipe weld interfaces, incompatible service media, gasket blocking the full flow bore
- Wrong bolt-circle positions, bolt counts, bolt-hole diameters or gasket interfering with bolt holes
- Wrong thread pitch, mismatched bolt/nut nominal size, off-axis fasteners
- Bolt shank too short for the nut or missing minimum thread protrusion
- Washers with insufficient clearance or seated away from flange faces
- Incorrectly marked removable weld-neck pipe/flange pairs
- An exploded view that separates permanently welded pipe and flange pieces
- Claiming manufacturer-certified/pressure-rated approval for the illustrative sample

The fit contract enforces geometry/layout **consistency**, not pressure-bearing design adequacy.

## Assembly exports

From repository root (Python 3.10+ and `jsonschema` installed):

```bash
python -m pip install "jsonschema>=4.20,<5"
python tools/asset_pipeline/validate_catalog.py
python -m unittest discover -s tests/catalog -p 'test_*.py' -v
python tools/asset_pipeline/export_assemblies.py --out dist/marine-assemblies
```

For every fully resolved assembly, the exporter produces:

```text
dist/marine-assemblies/
  assemblies.json
  ASM.LO.FLANGE.JOINT.DN50.ILLUSTRATIVE/
    assembled.glb      # glTF 2.0, hierarchy with independent named child meshes
    exploded.glb       # full static exploded state, preserving welded pairs
    assembled.obj      # portable high-detail 3D mesh
    exploded.obj
    ASM.LO.FLANGE.JOINT.DN50.ILLUSTRATIVE.mtl
    manifest.json      # BOM, source hash, output hashes, connection count, steps
    viewer.html        # self-contained WebGL inspector: open on Windows or Linux
    psp/
      chunk-000.ewmesh
      chunk-001.ewmesh
      ...
  ASM.FASTENER.M16.CLAMPKIT.ILLUSTRATIVE/
    [same output pattern]
```

`assembled.glb` and `exploded.glb` are full polygonal inspection models in **mm**. Each child instance is a named glTF node with `instance_id`, `component_id`, and `maintainable` metadata. Shared component meshes are reused within the GLB rather than cloned per fastener. The OBJ exports preserve the instance names in `o` groups.

The `psp/chunk-XXX.ewmesh` files are **baked** colour+XYZ geometry (metres) from the G2 low-poly catalog. Each stays under **1,800 triangles**, compatible with the native C++11 `Vertex3D` reader and existing PSP draw-batch limitations. They are **not yet installed in the runtime world**; no new `EBOOT.PBP` is necessary to inspect the G2b asset library.

### Self-contained interactive model viewer

Open `viewer.html` directly in Windows Edge, Chrome or another WebGL-capable browser. It has **no CDN scripts, external fonts or network requests**:

- Drag to orbit, scroll to zoom, **Fit view** to restore the camera
- Click components directly in the 3D viewport or search/select them in the left BOM list
- Drag the exploded-view slider to see the ordered component separation
- View individual part name, stable inventory ID, base material, replaceability, sizes and explosion step
- Read the persistent educational **not-for-fabrication** warning

This is a geometry/assembly inspector, not a maintenance procedure or certified training device. In the joint's exploded sequence the *welded* right pipe and flange move together; bolts, washers, nuts and gasket move separately for examination.

## Automated QA and acceptance

The new `tests/catalog/test_assembly_builder.py` verifies:

1. Catalog counts and successful fitted joint
2. Structural BOM/connection data
3. Gasket contact, bolt circle, thread pitch, bolt length, drilling clearance, pipe/gasket bore and welded-pair motion rejection tests
4. The GLB 2.0 container header, named node count and assembled/exploded transforms
5. Offline inspector payload, self-contained script, reproducible hashes
6. Per-chunk PSP output limits and real 16-byte C++11 vertex compatibility

The GitHub `catalog-contract` job exports G2b assembly artifacts and runs C++11 `verify_ewmesh` on every chunk. Existing G1/G2 geometry checks and full PSP/desktop/PPSSPP CI remain required.

## Next gates

**G2c:** Add user-facing part placement/attachment UX and a dedicated fastener/joint inspector with selectable disassembly operations, tightening sequence visualization, **without pretending bolt torque/tensile adequacy has been calculated**.

**G3:** Reconstruct floor and collision from a single architectural deck model, eliminating disappearing floor surfaces as a design-level constraint.

**G4:** Replace the placeholder LO pump/drive with a fully decomposable, electrically controlled and mechanically connected motor–pump–coupling assembly. Connect its systems to Missions 001/002.

**Merge rule:** G2b depends on approved G2 and G1. Do not merge this PR to `main` without explicit approval.
