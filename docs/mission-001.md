# Mission 001: Local lubricating-oil pump verification

## Fictional training scenario

A cargo vessel has an energized auxiliary bus. The duty engineer must ensure that a lubricating-oil pump starts from its local station and verify the resulting pressure at the Engine Control Room.

## Player journey

1. Spawn in the ECR next to the access door.
2. Approach the engine-room door and select it.
3. Walk to the LO pump; open its local control and request **START**.
4. Watch the pump transition through a short starting delay to **RUNNING**.
5. Allow discharge pressure to rise gradually, rather than instantly.
6. Return through the door to the ECR.
7. Approach and inspect the machinery monitoring console when pressure is at least 3.0 bar. This marks the mission complete.

## Engineering rules (prototype assumptions)

- Electrical bus is initially energized; pump start is inhibited if it is dead.
- Pump starts after 0.45 seconds of valid start demand.
- Steady pressure target is 4.4 bar in the simplified lumped model.
- Pressure rises with a 2.5-second time constant and decays with a 1.6-second time constant.
- After 7 seconds of unmet demanded pressure below 2.5 bar, a low-pressure alarm is raised.
- No low-pressure alarm is raised solely because a stopped, unrequested pump has zero pressure.
- Only ECR inspection with a running pump and actual pressure >= 3.0 bar completes the objective.
- An internal test hook can simulate electrical power loss or motor failure; user-facing fault injection is future work.

These mechanics illustrate causality and are *not* manufacturer specifications or real-world checklists.

## Acceptance tests

- No pump running without electrical power.
- No instantaneous full pressure on demand.
- Pump stop initiates pressure decay.
- Fault with active demand raises an alarm after a delay.
- Returning power allows recovery and clears the alarm after pressure recovers.
- Objective is incomplete until ECR inspection.
- Invalid and giant timestep values cannot destabilize the simulation.

## Verification still required

Check compiled `EBOOT.PBP` in PPSSPP at 480×272: font legibility, button mappings, equipment proximity interactions, screen transitions, pump toggle state, pressure visualization, and sound-free scene refresh.
