# ENGINE WATCH — PSP Roadmap

## Validation completed

- **v0.1.1:** Engine Control Room and walkable machinery platform, local LO pump controls, dynamic pressure and explicit ECR verification. PSPDEV cross-build passed, PPSSPP automated boot screenshot passed. User tested ECR and Mission 001 completion on Android PPSSPP.

## v0.2 — Current branch

- Two LO pumps, four isolation valves, developing suction-strainer restriction, main suction and strainer DP instrumentation, alarm acknowledgment and Mission 002 changeover/diagnosis.
- Updated compact top objective and contextual bottom controls for handheld readability.
- Mission 001 now requires explicit ECR verification. ECR **X** after pass advances to Mission 002.
- Headless tests for the complete fault, changeover, isolation and verification lifecycle.
- Pending: Android hands-on walkthrough, readability and controller mapping review.

## Planned iterations

- **v0.3:** Scalable fault/scenario definitions (JSON or compiled table), quick evaluation screen, scoring and save states; improve physical plausibility of LO system flow and temperature.
- **v0.4:** Freshwater cooling and two electrically powered pumps, local/ECR dependency mapping.
- **v0.5:** Starting-air compressors and receivers; main engine starting readiness checks and selected machinery alarms.
- **v0.6:** First-aid-style engineering decision exercises, emergency conditions, generator trips and changeover.
- **v0.7+:** 2.5D pre-rendered machinery environments with collision and better interaction proximity; sound effects; multiple deck navigation.

## Engineering fidelity

All pressures, response rates, delays, flags and alarms are currently illustrative. Do not use prototype data as actual marine engineering procedures. A manufacturer and vessel reference are required before claiming realistic quantitative operating behavior or training certification.
