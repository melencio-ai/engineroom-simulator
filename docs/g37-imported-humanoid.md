# G3.7 — Imported Quaternius Worker as a PSP Marine Engineer

**Stacked branch:** `feature/g3-7-quaternius-worker-import`, based on the G3.6 physical-control-socket branch. This is a review preview; do not merge the stacked chain without explicit approval.

## Why

The previous engineer was built from independent geometric primitives. G3.7 uses an **existing rigged and animated human asset** as the playable third-person body. The renderer still needs PSP-safe geometry and cannot consume a full `.glb` or Blender skeleton at runtime.

## Source and rights

- **Author:** Quaternius
- **Pack:** [Ultimate Modular Men Pack](https://quaternius.com/packs/ultimatemodularcharacters.html), released February 2022, explicitly marked CC0 on its pack page. The pack has 11 modular characters and 24 animations.
- **Character:** `Male/Worker.glb`, individually converted from original embedded-buffer glTF by [Ducz Character Creator](https://github.com/LuanDucate/Ducz.CharacterCreator) (developer credits Quaternius CC0).
- **Exact pinned Git blob:** `3979f271e62242b046bfd3c07d6962064c699eff`. Our importer refuses a different binary, preventing silent asset swaps.
- **Original complete pack:** [Creator's public Google Drive](https://drive.google.com/drive/folders/1USAAquX2JJWuA2m6zol0KUkFe3UkZ8zX?usp=sharing).
- **License clarification:** Quaternius's newer [general QAL v1.0](https://quaternius.com/license.html), published August 28 2026, restricts standalone redistribution of assets released under that newer license and says changes are not retroactive. The original 2022 pack page separately says **CC0**. Source/provenance is preserved in `tools/humanoid/quaternius_worker_source.json` and the build report.

The complete original GLB is **not checked into our game repository**. CI downloads and checks it at conversion time. The output is an optimized asset embedded as part of a playable simulator package, not repackaged as an independent stock-model collection.

## Build / conversion

1. `tools/humanoid/inspect_source.py` confirms real glTF 2.0 content, at least one humanoid skin, actual animations named Idle, Walk and Interact, and an exact Git blob fingerprint.
2. Headless Blender imports the genuine Worker skeleton/mesh, triangulates and decimates all outfit pieces to **at most 560 triangles** while preserving armature deformation.
3. Blender evaluates the imported armature for 6 Idle, 12 Walk and 8 Interact frames. It samples original material/texture colour at each output triangle, converts to PSP ABGR, scales to 1.82 m and removes source root displacement (the game controls movement).
4. The resulting `ENGINEER.EWM` EWHM-v1 stream contains packed ABGR + float XYZ vertices, with no runtime glTF parser or skinning; cap **1,800 vertices/frame and 1.35 MB total**.
5. The validator rejects nonmoving Walk clips, corrupt assets, nonfinite coordinates, unreasonable dimensions and budget overruns.
6. GitHub CI uploads this plus provenance and validation reports, then packages the **same ENGINEER.EWM** next to the PSP `EBOOT.PBP` for PPSSPP.

Example manual build (Blender installed locally):

```bash
mkdir -p build/humanoid assets/generated
curl -fL -o build/humanoid/Worker.glb https://raw.githubusercontent.com/LuanDucate/Ducz.CharacterCreator/main/public/models/Male/Worker.glb
python3 tools/humanoid/inspect_source.py --source build/humanoid/Worker.glb
blender -b -t 2 --python tools/humanoid/bake_worker.py -- --input build/humanoid/Worker.glb --output assets/generated/ENGINEER.EWM --manifest assets/generated/manifest.json --triangles 560
python3 tools/humanoid/verify_ewmh.py assets/generated/ENGINEER.EWM
```

## Runtime mapping

| Play state | Model |
| --- | --- |
| Third person standing | Imported Quaternius Idle clip |
| Third person walking/turning | Imported Quaternius Walk clip; phase driven by actual game distance |
| Third person climbing | Same Walk clip for now; source lacks a dedicated ladder-hold clip |
| Third person generic inspecting | Imported Interact clip |
| Third person **physically targeted LO button press** | Existing G3.6 procedural two-link arm IK until retargeted; preserves approximate glove-to-control alignment |
| First person | Existing G3.5.1 glove/palm/finger mesh and G3.6 reach target |
| Invalid/missing ENGINEER.EWM | Full previous procedural engineer and working gameplay |

**Current known limitation:** during physically targeted 3P button press the body briefly changes to the procedural fallback to retain the precise target. The next retargeting gate should use the imported skeleton throughout, not fabricate a new rig. Uniform colour is flat/low-poly rather than an original UV-textured render. Helmets/PPE remain illustrative, not certified.

## Preview on Windows or phone

Download the CI artifact `engine-watch-psp-g37-quaternius-worker-preview` into `D:\Downloads`. It contains BOTH:

```text
PSP/GAME/ENGINEWATCH/EBOOT.PBP
PSP/GAME/ENGINEWATCH/ENGINEER.EWM
```

Do not copy only the EBOOT: the sidecar is what makes this the *imported* character build. On Windows PPSSPP, close PPSSPP and run the branch's `tools/install-g37-ppsspp.ps1`, which backs up both EBOOT and previous character files. For Android/iPhone, copy both files into the actual PPSSPP Memory Stick `PSP/GAME/ENGINEWATCH/` folder.

Start a fresh game, press **SELECT** for 3P, view the Worker in the ECR, walk the inclined ladder and visit Pump Flat. The third-person camera stays G3.5.2's obstruction-aware version.

## Acceptance criteria

- All native scene, G3 deck/camera/mission and G3.6 interaction CTests pass.
- Imported clip tests verify real rig movement, bounded triangles and fallback for a corrupt asset.
- CI fetches source and validates SHA, bakes all three genuine source clips in Blender, and passes the EWHM verifier.
- PSP build and PPSSPP boot smoke pass with BOTH files installed.
- Human review on phone confirms the model has proper character silhouette, orientation, feet above deck and acceptable animation.
- Do not merge until visual QA and owner's explicit approval.

**Next:** retarget real Worker hands and ladder grip to G3.6 sockets (per-bone contact IK without switching to procedural body); add naval PPE variants and a helmet attachment on the rig.
