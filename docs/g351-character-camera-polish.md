# G3.5.1 — Character Camera and Low-Poly Anatomy Polish

Branch: feature/g3-5-1-character-camera-polish
Parent: G3.5 marine engineer PR #13, stacked on G3 deck-constructor PR #12.
Status: preview; unmerged; illustrative low-poly character and PPE.

## Changes

1. Remain in full-body 3P throughout inclined-ladder and corridor movement. The old G3.5 chase camera abruptly returned to first-person hands when collision shortened the camera boom. G3.5.1 instead keeps the body rendered and displays VIEW 3P CLOSE when camera placement is constrained.
2. Smooth outward chase-camera extension and aim tracking. A nearby obstruction shortens camera distance promptly to avoid penetration; 20 collision probes reuse physical G3 deck/stair/machinery rules.
3. Aim approximately 1.25 metres beyond the character, moving the crosshair away from the helmet. Use adaptive third-person FOV (76–91 degrees) to reduce extremely large close-up framing, shared between desktop and PSP.
4. Replace box-shaped skull and chest with a faceted head and tapered boiler-suit shoulders/waist, with a modest reduction in the walking-leg pendulum and visible 3P glove/thumb details.
5. Move camera-space first-person gloves above the PSP bottom HUD. Model a palm, wrist cuffs, four separate fingers and a thumb per glove. Keep the character dynamically rendered, never baked into static world geometry.

## Automated acceptance

- Tests check first-person glove projection in 480x272 pixels: fingers/palms visible above the 226px HUD; crosshair at screen center stays clear.
- Tests traverse the actual ECR bridge and forward inclined ladder while in 3P, asserting that show_body never becomes false and the HUD mode matches rendering.
- All 5 native CTest suites, LO pressure/pump missions, G1–G3 source contracts, PSP/desktop compilation and PPSSPP boot smoke are required.
- All character vertices must fit the existing independent 1,800-vertex runtime budget. PPE is illustrative and not approved for shipboard tasks.

## PPSSPP preview

Download the GitHub Actions artifact engine-watch-psp-g351-character-camera-preview into D:\Downloads. The companion installer tools/install-g351-ppsspp.ps1 automatically backs up the previous EBOOT and does not need GitHub CLI.
On a phone, extract that PSP artifact and copy the EBOOT.PBP into the PPSSPP PSP/GAME/ENGINEWATCH directory.

Check Deck 04 ECR, bridge, stair descent, Pump Flat and close bulkheads while changing views with SELECT. The camera must not abruptly revert to first person and the hands must not cover the lower HUD or crosshair.

## Limitations

The chase camera uses navigation and machinery proxies, not a complete per-triangle occlusion mesh. The prototype hands are still segmented rigid geometry, without detailed natural skinning, true IK or time-synchronized fingertip contact. PPSSPP boot smoke cannot replace moving-camera human visual verification.

Do not merge PR #14 or stacked dependencies to main without explicit user approval.
