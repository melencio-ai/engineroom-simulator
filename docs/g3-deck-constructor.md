# G3 — Shared Engine Room Deck Constructor (PSP preview)

**Branch:** `feature/g3-deck-constructor-shared-layout`  
**Base:** [G2b PR #11](https://github.com/melencio-ai/engineroom-simulator/pull/11) (stacked on G2 PR #10 and G1 PR #9)  
**Purpose:** eliminate independent floor/collision definitions that allow walkable positions with incomplete rendered deck geometry.

## Why this exists

Earlier `World3D.cpp` maintained three unrelated descriptions of deck reality:

1. Hard-coded `deckSlab()` plates and upper-shaft gaps.
2. Independently placed reusable `OpenSteelGrating` meshes and a Pump Flat maintenance cover.
3. Separate numerical tests in `canStand()` for hull limits, engine well, ECR bridge and stair cuts.

They were intended to match but the game had no way to prove they matched. G3 introduces **one physical construction plan** and derives all three outputs from it. Mechanical machinery obstacles remain independent, as they should: a real steel deck can exist underneath an engine foundation even though the player cannot walk through its machinery.

## Physical source of truth

`src/scene/DeckLayout.hpp` and `DeckLayout.cpp` describe:

| Entity | Count | Meaning |
| --- | ---: | --- |
| Solid steel plates | 46 | Physical, top-faced walkable deck panels |
| Open grating cells | 32 | Grating meshes with separate collision support, not solid slabs beneath |
| Inset service cover | 1 | Pump Flat steel chequer maintenance cover at deck height |
| **All deck cells** | **79** | Non-overlapping spatial floor definition across five levels |
| Upper openings | 12 | 4 engine wells, 4 forward stair shafts, 4 aft escape shafts |
| Grating bands | 8 | 4 decks × 2 edges, each comprising four contiguous bays |
| Stair paths | 2 | Forward and aft inclined ladder flights shared with scene geometry |

The Level 00 Tank Top is a continuous steel plate under both inclined stairs. The stair sweep is kept out of **flat** walking collision; the actual stair height is still resolved with `World3D::stairHeight()`. Upper shaft openings are genuine absent floor cells. The ECR floor and narrow access bridge are defined in the same plan as the rest of Deck 04.

### Runtime construction

`World3D::build()` now iterates `DeckPlan::cells()`:

- `SolidSteelPlate` → one normal thick green steel deck section.
- `OpenSteelGrate` → exactly one reusable low-poly open grate with metal top at deck elevation.
- `InsetServiceCover` → exactly one chequer-plate catalog object; no hidden second steel slab.

The support framework is driven by `DeckPlan::gratingBands()`, and the inner-well guardrails are driven by `DeckPlan::openings()`.

The reusable grating mesh spans only `-0.49..+0.49` in both horizontal local axes, so its true normalized width is **0.98**, not **1.00**. The previous object transform used the nominal panel size directly and left approximately **45.5 mm** total gap between two adjoining 2.275 m bays. G3 calculates each transform scale as **bay dimension / 0.98**; consecutive panels now meet exactly with no missing strip along the seam.

This keeps the existing **16-bit PSP depth precision**, floor top elevations, 33 older reusable component archetypes, pump positions, machinery and mission simulation.

## PSP renderer camera-cutout fix — November 19

The first G3 screenshot review showed **entire walls and broad green deck faces disappearing when the player walked or turned close to machinery**. The cause involves PSP hardware clipping in addition to the physical deck layout.

- **Near-plane enable:** The PSP renderer was missing `sceGuEnable(GU_CLIP_PLANES)`. Without it, PSP can discard a triangle in its entirety if any vertex passes behind the camera. G3 now explicitly enables that state during GE initialization AND every 3D frame.
- **Guard-band-safe shell panels:** PSP only implements near clipping in hardware, not full left/right/top/bottom clipping. A giant wall or floor triangle whose projected vertex escapes screen coordinates 0..4096 can disappear even with near clipping enabled. The new `panelizedBox()` subdivides only six exposed outer faces of oversized hull/ceiling boxes into approximately 2-m squares and large steel-deck slabs into approximately 3-m squares (the deck is at least 1.65 m from the standing camera). It does not add overlapping interior faces, and the floor subdivision is tuned to stay within the 129,999-vertex PSP scene budget.
- **No collision-model change:** DeckPlan, pump/stair routes, vertex colour layout, reversed 16-bit depth, 0.25-m near plane and the 60-m far plane are preserved.
- **Regression coverage:** `tools/verify_psp_render_state.py` asserts near clipping stays enabled before initialization and each 3D draw. `tests/World3DTests.cpp` checks that the major structural wall/ceiling planes are covered by short triangles rather than oversized guard-band-risk faces.

**Limit:** Automated native geometry tests and PPSSPP boot smoke cannot prove the rendering issue is gone at every viewpoint. A real camera sweep along the Pump Flat walls, beneath the upper decks, and at the ECR stair landing is still required before merging.

References: [PPSSPP GE clipping behavior](https://www.ppsspp.org/docs/psp-hardware/gpu/ge-vertex-pipeline/) and [PSPSDK GE states](https://pspdev.github.io/pspsdk/group__GU.html).
 To avoid shallow-angle depth conflicts in that 16-bit buffer, G3 removes the old **4-mm-above-floor decorative plate-seam quads**. Future panel seams should be represented by embedded/baked surface material details rather than competing near-coplanar floor polygons.

### Collision from the same source

`World3D::canStand()` invokes `DeckPlan::supportsFootprint()` before checking machinery obstacle rectangles. The flat-walking footprint checks the union of adjacent floor cells, not each panel separately, so crossing a plate-to-grate joint does not create an invisible wall. If the footprint overlaps a real open well or shaft, floor support is rejected. Stair movement uses the existing continuous slope solver.

**Important:** the union-of-cells proxy approximates a grating as a continuous support surface for the player; the drawn grating still has real openings. This is standard game-physics simplification, not an engineering load-bearing analysis.

## Portable constructor export

The project now includes `tools/export_deck_plan.cpp` for creating inspectable planning artifacts directly from the C++ source of truth:

```bash
cmake -S . -B build/deck-export \
  -DENGINEWATCH_BUILD_GAME=OFF \
  -DBUILD_TESTING=ON \
  -DENGINEWATCH_EXPORT_DECKPLAN=ON

cmake --build build/deck-export --parallel 2
ctest --test-dir build/deck-export --output-on-failure

mkdir -p dist/deck-constructor
./build/deck-export/enginewatch_export_deckplan dist/deck-constructor
```

Outputs:

```text
dist/deck-constructor/
  deck-constructor.json  # five levels, cell bounds, openings and grating bands
  deck-00.svg            # Tank Top schematic
  deck-01.svg            # Pump Flat schematic
  deck-02.svg            # Main Engine level
  deck-03.svg            # Auxiliaries
  deck-04.svg            # ECR Upper + access bridge
  README.txt
```

The SVG views use real world **metres** and clearly distinguish steel plate, open grate, maintenance cover and genuine floor openings. These are construction **schematics of a fictional composite ship**, not class-approved vessel GA drawings.

Download CI artifact **`engine-watch-deck-constructor-g3`** to inspect the plans without needing to run a C++ build.

## Tests

`tests/DeckLayoutTests.cpp` adds an independent `constructed_deck_geometry_and_collision` CTest suite:

- validates all 79 deck cells, 32 grating panels, one cover and 12 genuine shaft/well openings
- proves every solid plate cell has an actual top-face triangle at its declared elevation
- proves each grate is backed by exactly one catalog placement, scaled to fill 100% of its declared bay at the correct height
- proves each upper stair opening and the central machinery well have NO floor cell
- checks plate/grate transition, grate-bay seams, ECR bridge, and the Pump Flat maintenance-cover perimeter are traversable
- checks both solid gallery floor surfaces remain present along multiple positions on all five decks
- preserves existing Deck 04 → Deck 00 stair traversal and Mission 001 LO pump simulation regressions

The complete EBOOT and desktop build, catalog schemas, parametric assembly model tests, object export and PPSSPP boot smoke all remain mandatory.

### Windows PPSSPP preview

The CI EBOOT artifact is named **`engine-watch-psp-g3-deck-constructor-preview`** and the matching installer lives at **`tools/install-g3-ppsspp.ps1`**. No GitHub CLI is necessary. Close PPSSPP, download the artifact ZIP into `D:\Downloads`, run the installer from the G3 feature branch, then start a *new game*, not an old savestate. The installer preserves an EBOOT backup.

### Manual release gate

PPSSPP boot smoke proves the title starts and renders an initial framebuffer, **not** that all floor surfaces remain visible while moving. Before merge, manually walk and capture screenshots/videos at:

1. Deck 04 ECR bridge and forward stair entrance.
2. Deck 03 port/starboard steel-to-grating transitions and bay seams.
3. Deck 02 main-engine gallery, looking down into the machinery well.
4. Deck 01 LO pump station, maintenance cover, gauge panel, grating strips.
5. Deck 00 tank top, including lower stair landing.

The player must not appear to float, fall through a supported floor, see a panel disappear mid-step, or encounter invisible barriers at ordinary plate/grating seams.

## Next step

G4 will assemble the real 3D low-poly LO pump/motor skid from the validated G1–G2b physical catalog, adding exploded maintenance/inspection models separately from playable gameplay. G3 supplies the stable structural base; it does not add pressure-vessel engineering approval.

**Do not merge PR #12 (or its stacked dependencies) to main without explicit owner approval.**
