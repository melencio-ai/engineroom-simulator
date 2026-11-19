# Mission 001: Local lubricating-oil pump verification

## Fictional training scenario

A cargo vessel has an energized auxiliary bus. The duty engineer must ensure that a lubricating-oil pump starts from its local station and verify the resulting pressure at the Engine Control Room.

## Player journey

1. Spawn on **Deck 04, ECR upper platform** with a view toward the 3D machinery well.
2. Walk through the central ECR doorway and use either inclined steel stair flight. **Walk down the treads physically** from Level 04 to Level 01, following the clear bypass at the landing to reach each next flight; no Cross-to-climb panel is needed.
3. Locate the **LOCAL LO PUMP STATION** on Deck 01, press **X**, keep **MAIN PUMP MOTOR** selected, and press **X** again to request START.
4. Watch the pump transition through a short starting delay to **RUNNING**.
5. Allow discharge pressure to rise gradually, rather than instantly.
6. Physically walk back up the inclined steel stair flights to Deck 04 and return through the ECR doorway.
7. Approach the machinery monitoring console when pressure is at least 3.0 bar. Press **X** to open the console, and **X** again to explicitly verify the readings. Mission 001 is complete. A further **X** at the console starts Mission 002.

## Engineering rules (prototype assumptions)

- Electrical bus is initially energized; pump start is inhibited if it is dead.
- Pump starts after 0.45 seconds of valid start demand.
- Steady pressure target is 4.4 bar in the simplified lumped model.
- Pressure rises with a 2.5-second time constant and decays with a 1.6-second time constant.
- After 7 seconds of unmet demanded pressure below 2.5 bar, a low-pressure alarm is raised.
- No low-pressure alarm is raised solely because a stopped, unrequested pump has zero pressure.
- Only an explicit ECR **VERIFY** action with the main pump running and actual pressure >= 3.0 bar completes the objective.
- The engine core includes test hooks for electrical failure and motor failure. The playable Mission 002 introduces a developing main suction strainer restriction.

These mechanics illustrate causality and are *not* manufacturer specifications or real-world checklists.

## Acceptance tests

- No pump running without electrical power.
- No instantaneous full pressure on demand.
- Pump stop initiates pressure decay.
- Fault with active demand raises an alarm after a delay.
- Returning power allows recovery and clears the alarm after pressure recovers.
- Objective is incomplete until explicit ECR verification.
- Invalid and giant timestep values cannot destabilize the simulation.

## Verification

Versions v0.1/v0.2 demonstrated the mission process in a 2D engine room. The v0.4 route now crosses multiple 3D decks. Automated core simulation and 3D navigation tests and PPSSPP boot checks exist; the full physical route still requires a hands-on PSP/PPSSPP walkthrough.
