# ENGINE WATCH roadmap

## v0.1 — First playable vertical slice
- [x] Portable machinery simulation core
- [x] Two traversable rooms with functional door transitions
- [x] Interactive local LO pump panel
- [x] Simulated pump start delay, LO pressure ramp and decay
- [x] ECR status display using the same machinery state
- [x] Mission 001 completion after ECR verification
- [x] Alarm condition with simple delay and recovery rules
- [x] Host simulation tests and CI cross-build definition
- [ ] Verify PSP CI job successfully produces an EBOOT
- [ ] Physically verify controls, fonts, readability, and performance in PPSSPP / PSP hardware

## v0.2 — Machinery depth
- [ ] Pump motor and starter failure cases exposed via missions
- [ ] Valve state, strainer blockage, pressure/flow sensors
- [ ] Alarm acknowledgement and event log
- [ ] Equipment interaction collision boundaries

## v0.3 — Main engine pre-lube and cooling
- [ ] Lube oil supply and temperature dynamics
- [ ] Freshwater cooling circuit and electrical dependencies
- [ ] Main engine readiness states and authentic, model-specific interlocks

## v0.4 — Air and starting system
- [ ] Compressors, receivers, and pressure consumption
- [ ] Main engine starting interlock demonstration

## v0.5+ — World and missions
- [ ] Higher-fidelity pre-rendered 2.5D machinery views
- [ ] More ECR screens / generator and switchboard systems
- [ ] Scenario injection and scored engineering watch tasks
- [ ] Persistence and audio

**Important:** The illustrative numbers in v0.1 are *not* normative operating limits. Future simulation specifications must document engine model, approved references, and explicit deviations.
