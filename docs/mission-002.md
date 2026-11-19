# Mission 002 — Low Lubricating Oil Pressure

## Scenario
Following successful ECR verification of Mission 001, the player selects **X: START MISSION 002** in the ECR console. A restriction develops in the **main LO pump suction strainer**. Suction differential pressure rises, main pump inlet pressure falls, and the common LO header pressure gradually deteriorates. An alarm appears after sustained low pressure.

**Training objective:** diagnose the hydraulic restriction and carry out a controlled changeover to the available standby pump; secure and isolate the affected main pump; verify normal pressure and cleared alarm at the ECR. The same machinery simulation is shared by the local controls and ECR.

## Player experience

1. Complete Mission 001 (operate the main LO pump, then explicitly verify in ECR).
2. In ECR, press **X** again to start Mission 002. Monitor the header reading and low-pressure alarm.
3. From **Deck 04 ECR**, walk down three levels via either inclined steel stair route to **Deck 01 Pump Flat**. Approach the main LO suction strainer in the forward port-side machinery bay and press **X** to inspect differential and suction pressure. Inspection of the developing restriction is recorded.
4. Approach the **LOCAL PUMP STATION**, press **X**, navigate the six controls using **D-pad Up/Down**, and press **X** to operate the highlighted control.
5. Open standby **suction and discharge** valves, then start the standby pump. Confirm the header pressure recovers.
6. Stop the main pump and close its suction and discharge valves to isolate the affected path.
7. Walk up the inclined steel stair flights and return to **Deck 04 ECR**. Acknowledge a still-active low-pressure alarm with **X**, and press **X** again when conditions are normal to verify restoration. If the alarm has cleared after timely intervention, acknowledgment isn't required.

The assessment requires the strainer diagnosis to be recorded, standby pressure stable for at least two seconds, main pump stopped and isolated, and the ECR verification command. A user can recover before the alarm has time to activate.

## Controls

- Analog: walk/strafe; D-pad Left/Right: turn; L/R: look down/up; D-pad Up/Down: walk or select rows in a local panel; use the inclined stairs directly by walking.
- X: interact, toggle a selected pump/valve, acknowledge a live alarm, verify mission, or advance.
- O: close a panel.
- Triangle: help.
- Start: pause, then **X**: reset both missions.

## Simulated equipment

- Electrical supply permissive shared by both motors.
- Main and standby pump start delay and individual running state.
- Main/standby suction and discharge shutoff valves.
- Main suction strainer DP and main pump suction pressure.
- Shared LO header pressure with first-order pressure build/decay dynamics.
- Simplified LO oil temperature and low-pressure alarm with acknowledgment.

## Limitations

This is a **gameplay-oriented illustrative process model**. It does not simulate actual pump curves, valve lineups, flow continuity, real thermal/hydraulic transients, or manufacturer-approved procedures. The simulation values and alarm thresholds are not suitable for a real vessel. The game environment is a five-height-band, first-person low-polygon 3D machinery space; the layout and models are composite representations, not actual ship drawings. Standby pumps with shut valves can be commanded on here to illustrate hydraulic dependency; real interlocks, limits, and safe operation must follow specific vessel procedures.
