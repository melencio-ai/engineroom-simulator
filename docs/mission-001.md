# Mission 001: Local lubricating-oil pump verification

## Fictional training scenario

A cargo vessel has an energized auxiliary bus. The duty engineer must ensure that a lubricating-oil pump starts from its local station and verify the resulting pressure at the Engine Control Room.

## Player journey

1. Spawn in the ECR next to the access door.
2. Approach the engine-room door and select it.
3. Walk to the **LOCAL PUMP STATION**, press **X**, keep **MAIN PUMP MOTOR** selected, and press **X** again to request START.
4. Watch the pump transition through a short starting delay to **RUNNING**.
5. Allow discharge pressure to rise gradually, rather than instantly.
6. Return through the door to the ECR.
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

The original v0.1.1 visuals and the mission completion path were demonstrated on Android PPSSPP. Version v0.2 modifies control panels and progression and requires a new hands-on walkthrough. The game builds through PSPDEV CI, with a PPSSPP rendering smoke check.
