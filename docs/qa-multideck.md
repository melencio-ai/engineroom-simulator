# v0.4 PPSSPP Windows - Multi-deck Acceptance Checklist

Run against a **fresh EBOOT launch**, not an old v0.3 PPSSPP save state.
Source branch: `feature/psp-v0.4-multideck-environment`.
Build artifact: `engine-watch-psp-v0.4-multideck` (GitHub Actions, successful build).
Copy `PSP/GAME/ENGINEWATCH/EBOOT.PBP` into the *configured* PPSSPP Memory Stick folder.

## 1. Start scene and HUD

- [ ] Launches without `Could not find executable ms0:/PSP/GAME/...`.
- [ ] Scene opens on **Deck 04 ECR**, looking through the upper machinery doorway.
- [ ] HUD title reads **ENGINE WATCH 3D | 04 ECR / UPPER** and pressure/mission text is visible (font UV bug fixed).
- [ ] Main engine reaches upward through open levels; catwalks are partial and clearly separated.
- [ ] Upper ECR monitors are accessible when looking back toward the control wall.

## 2. Horizontal / vertical movement

- [ ] PSP analog stick walks and strafes. D-pad left/right turns camera.
- [ ] L trigger looks downward; R trigger looks upward. Looking down from Deck 04 shows lower open platforms.
- [ ] Walk through actual ECR doorway; walls cannot be walked through.
- [ ] Main engine, pumps, generator sets and outer bulkhead block player appropriately.
- [ ] Approach **yellow service ladder** on the forward starboard side near X -0.75, Z +3.7; press Cross to open ladder panel.
- [ ] Choose **DOWN**, press Cross, see brief continuous vertical motion, arrive at Deck 03 with deck HUD updated.
- [ ] Repeat down to Deck 02, Deck 01 and Deck 00; verify the main engine appears at corresponding height on each.
- [ ] Choose **UP** to ascend and return to Deck 04.
- [ ] Second ladder at aft port side also connects all decks.
- [ ] The top deck cannot climb further upward; Level 00 cannot descend below tank top.
- [ ] You cannot accidentally walk into the open central engine well or ladder shaft holes.

## 3. LO pump missions

- [ ] Navigate to **Deck 01 Pump Flat**. Locate two LO pumps and their local panel.
- [ ] Cross at the panel opens six pump/valve actions.
- [ ] Mission 001: start main LO pump; observe delayed pressure buildup.
- [ ] Return to **Deck 04 ECR**, face console and verify stable LO pressure.
- [ ] Mission 002: activate from ECR. Return to Deck 01, inspect strainer DP, line up standby suction/discharge, start standby pump, stop and isolate main, return to ECR to verify.
- [ ] Pressure values/alarms reflect machine state regardless of which deck is visible.

## 4. Performance and UX

- [ ] No blank screen, incorrect clipping, inverted 3D, missing fonts, or repeated 2D floor map.
- [ ] Review camera pitch/turn rates and narrow catwalk collision. Note where controls become difficult.
- [ ] Verify ladder traversal is visually understandable (current implementation is automated vertical climb without hand animation).
- [ ] Test on PPSSPP Windows at 1x and 2x rendering resolution; document any drops or visual artifacts.
- [ ] Take screenshots from L4 ECR window, L3 generators, L2 engine, L1 pump flat and L0 tank top.
- [ ] Pause and reset, then verify spawn returns to the ECR at Level 04.

## Scope and gate

Automated CI only establishes that native PSP EBOOT builds, desktop compiles, headless gameplay tests pass, and a PPSSPP launch/framebuffer smoke test succeeds. **It does not establish game-feel, visual accuracy, hardware performance or complete mission completion.** Obtain hands-on review before merging.

Note: All machinery, alarm and process values are illustrative and not ship-specific operating instructions.
