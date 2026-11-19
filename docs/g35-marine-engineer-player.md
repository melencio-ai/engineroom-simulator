# Engine Watch G3.5 — Marine Engineer Player

**Branch:** feature/g3-5-marine-engineer-player  
**Parent:** G3 deck constructor PR #12 (stacked on PRs #9–11)  
**Status:** Prototype player character and camera, not merged to main.

## Goal

Move beyond an invisible 1.65-m first-person camera. The marine engineer now
has a low-poly body, boiler suit, PPE silhouettes, joint-driven animation, and
the option to switch between first and third person. Existing marine machinery,
G3 stable deck geometry, mission logic and renderer clipping are preserved.

## Delivered

- Procedural C++11 character approximately **1.82 m tall**, eye **1.65 m**.
- Navy boiler suit, reflective bands, hard hat, work gloves, footwear, ID badge
  and radio silhouette. PPE is illustrative and not certified for any task.
- Rigid hierarchical hip/knee/shoulder/elbow joint pivots; idle, walking,
  inclined-ladder climbing, inspection and control reach prototypes.
- Locomotion is driven by **actual distance travelled** so blocked movement
  cannot create treadmill walking animation.
- First-person sleeves/gloves follow camera yaw and pitch; third-person shows
  the full body behind a shortened collision-aware chase camera.
- PSP / PPSSPP / phones: **SELECT** toggles the camera. Desktop: **V**;
  controller: **Back/View**.
- LO pump and ECR menu operations visually trigger inspection or reach
  gestures but retain all current simulation state transitions.
- Animated body and hands are **not appended to static ship vertices**;
  the PSP allocates a separate transient GU vertex batch and flushes cache.

This is **segmented low-poly rigid animation**, not a smooth skinned rig.
It does not yet implement fingertip IK, accurate handrail grabbing, timed
physical control-contact events or task-certified PPE. The chase camera uses
physical deck and machinery obstacle proxies; it does not raycast against
every narrow decorative pipe and must be manually visually tested.

## Character OBJ exports

The native tool **tools/export_player_character.cpp** produces:

    idle_body.obj
    walking_body.obj
    ladder_body.obj
    pushbutton_reach.obj
    instrument_inspect.obj
    first_person_hands.obj
    marine_engineer.mtl
    profile.json
    README.txt

Each file is a separate OBJ mesh snapshot of the exact C++ game model,
rather than a rigged or animated master GLB. Units are metres.

Build and export:

    cmake -S . -B build/character -DENGINEWATCH_BUILD_GAME=OFF -DBUILD_TESTING=ON -DENGINEWATCH_EXPORT_CHARACTER=ON
    cmake --build build/character --parallel 2
    ctest --test-dir build/character --output-on-failure
    mkdir -p dist/marine-engineer
    ./build/character/enginewatch_export_character dist/marine-engineer

CI artifact: **engine-watch-marine-engineer-character-g35**.

## Install the G3.5 PSP preview

Use GitHub Actions to download **engine-watch-psp-g35-marine-engineer-preview**
to D:\Downloads. Close PPSSPP before installation.

    $downloads = "D:\Downloads"
    $installer = Join-Path $downloads "install-g35-ppsspp.ps1"
    Invoke-WebRequest -Uri "https://raw.githubusercontent.com/melencio-ai/engineroom-simulator/feature/g3-5-marine-engineer-player/tools/install-g35-ppsspp.ps1" -OutFile $installer
    powershell.exe -NoProfile -ExecutionPolicy Bypass -File $installer -Downloads $downloads

The script automatically backs up the previous EBOOT. Start a fresh game
and press **SELECT** on PSP/PPSSPP or the phone's virtual PSP controller
to switch between first-person hands and third-person engineer.

## Automated acceptance

**tests/PlayerCharacterTests.cpp** asserts:

1. Rig/hands contain only finite ABGR/XYZ triangle vertices and stay within
   1,800 transient vertices per character rendering pass.
2. Body/helmet height, feet contact and approximately human proportions.
3. Movement alters limb geometry, blocked walking does not advance gait,
   and ladder climbing changes pose.
4. First-person hands remain beyond the PSP 0.25-m camera near plane.
5. Camera toggles do not teleport, reset missions or inflate the static PSP
   vertex budget.
6. ECR station inspection and button press gestures preserve existing
   Mission 001/002 logic.
7. All G1–G3 catalog, structural, desktop, PSP and emulator checks stay green.

## Human screenshot and playthrough checklist

- Walk ECR Deck 04 with first-person gloves visible but not blocking controls.
- Press SELECT; inspect boiler suit, helmet, body posture and walking cadence.
- Descend Deck 04 → 03 → 02 → 01 on the actual inclined steel ladders.
- At narrow ECR doorways and machinery, the camera shortens or falls back to
  hands rather than entering known colliders.
- Approach LO pump control board: open menu, press main pump start, confirm
  reaching animation and intact pressure/missions.
- Repeat on Android/iOS PPSSPP using virtual PSP SELECT.

**Next:** G3.6 connects character hand targets to button/valve/gauge sockets.
G4 constructs fully detailed main and standby LO pump assemblies from the
existing reusable component catalog.

**Do not merge PR #13 to main** without full CI, human PPSSPP/phone visual
QA, explicit owner approval and required prior stacked merges.
