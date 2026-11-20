# G3.7.1 — Marine Engineer Human Model Quality

**Parent:** G3.7 Quaternius Worker [draft PR #17](https://github.com/melencio-ai/engineroom-simulator/pull/17). **This branch:** [draft PR #18](https://github.com/melencio-ai/engineroom-simulator/pull/18). Neither is merged to main.

## User-recording defects

The G3.7 PPSSPP review captured fragmented human geometry and dark/light checkerboard patches on the body. The third-person model was recognizable as a Worker, but its disproportionately simplified limbs and messy head needed quality control. A nearby overhead white structural panel also obstructed part of the camera view.

### The measured decimation problem

The previous export took **5,240 source triangles to 542** (about 10% geometry). In the first attempted G3.7.1 weighted allocation, the original 4 meshes resulted in **441 head triangles, 286 body, 95 legs and 53 feet**. That was too head-heavy to preserve a continuous boiler-suit silhouette.

The corrected converter uses the pinned Worker's actual mesh names, **Worker_Body / Worker_Legs / Worker_Head / Worker_Feet**, with dedicated budgets of about **50% / 20% / 22% / 8%**. The target is **900 source-model triangles** plus 40 lightweight animated helmet triangles. It is deliberately a *separate* character buffer, so it does not inflate the nearly full static PSP ship scene.

## Implemented changes

1. **Preserve geometry:** Blender's modifier decimates the four original skin-weighted pieces independently and retains enough leg, foot, head and torso geometry to avoid the previous low-detail holes. The conversion report lists each part's input, allocated and resulting triangle counts.
2. **Consistent marine workwear:** Instead of sampling unrelated points on the texture atlas after heavy decimation, paint actual original mesh and skin-bone regions using a coherent maritime color palette: high-visibility orange coveralls, darker gloves and safety boots, normal skin, and a yellow safety helmet. This is *visual* PPE, not safety certification.
3. **Added helmet:** A 40-triangle faceted crown and brim follows the imported, animated head in each baked frame. It is made from the real face/head position rather than a detached permanent world object.
4. **Animation integrity:** Validate that Blender's evaluated polygon/triangle order is identical across every original **Idle (6), Walk (12) and Interact (8)** frame. Do not rely solely on matching triangle counts. Normalize X/Z around the animated pelvis/hips instead of each frame's moving extremity bounding box; align each frame to the walking floor.
5. **Automated geometry gates:** Verify per-frame height, broken/degenerate faces, implausibly long triangles, frame-to-frame correspondence, limited palette colors, and that leg/body/foot/head geometry and helmet survive optimization. The 3,000-vertex dynamic PSP character limit remains independent of ship geometry.
6. **Obstruction-aware camera improvement:** Score the sightline from each third-person camera candidate to the player's look-ahead and chest targets, not just one engineer-to-camera ray. This reduces chances of viewing a nearby overhead bulkhead or beam while the character line of sight is otherwise technically clear.

## PSP resources and boundaries

The Quaternius asset remains the **CC0 Worker from the 2022 Ultimate Modular Men pack**. The original GLB source is verified by an immutable Git blob hash before conversion and is **not checked into the Engine Watch repository**.

Runtime format stays `ENGINEER.EWM` with 16-byte ABGR + XYZ vertices, up to 26 source-animation frames, 3,000 triangle-list vertices per frame (1,000 triangles), and at most **1.35 MB** for all frames. Our C++11 runtime uses CPU vertex-frame interpolation rather than PSP GPU skinning. PSP GU rendering remains depth-tested and uses the same fixed near-plane clipping as before.

The LO pump control and G3.6 targeted button press still use the existing procedural IK fallback until the Quaternius skeleton can be retargeted to physical interaction sockets. The imported model is for idle, walk and generic interaction poses; ladder movement still borrows a walk clip.

## Build acceptance

CI must pass native marine components, seven C++ scene/character/equipment tests, Blender source validation, animated LOD and PPE checks, PSP compilation, and PPSSPP boot smoke with BOTH `EBOOT.PBP` and `ENGINEER.EWM` installed. **Manual moving-camera visual QA remains required**; booting the game alone does not prove that anatomy looks correct.

### Installation

From this branch's successful GitHub Actions run, download the `engine-watch-psp-g371-humanoid-quality-preview` artifact into `D:\Downloads`. Close Windows PPSSPP and run:

```powershell
$downloads = "D:\Downloads"
$installer = Join-Path $downloads "install-g371-ppsspp.ps1"
Invoke-WebRequest -Uri "https://raw.githubusercontent.com/melencio-ai/engineroom-simulator/feature/g3-7-1-humanoid-quality-lod/tools/install-g371-ppsspp.ps1" -OutFile $installer
powershell.exe -NoProfile -ExecutionPolicy Bypass -File $installer -Downloads $downloads
```

The installer backs up the existing EBOOT and character sidecar. For Android PPSSPP, extract BOTH `PSP/GAME/ENGINEWATCH/EBOOT.PBP` and `PSP/GAME/ENGINEWATCH/ENGINEER.EWM` to the device's active PPSSPP Memory Stick.

## Human review checklist

- HUD shows `QTR` when the imported model is loaded, not `PROC`.
- Press SELECT to 3P and inspect the complete engineer from front, side and rear. Head, arms, legs, boots and hard hat should be recognizable, without detached black-and-white patches.
- Walk ECR bridge, turn slowly near the white overhead beam, then descend the inclined steel stairs. No floor/wall disappearance; camera attempts a clear shoulder before looking through the beam.
- At Deck 01 Pump Flat, check LO pump station menu and fingertip reach. A brief procedural body during physically aimed button animation is a **known limitation** until G3.8 rig/socket retargeting.
- Record a short G3.7.1 video and compare with the prior 24-second G3.7 clip before approving anything for merge.

**No merges to main or stacked parents without explicit approval.**
