# Accident detection (staged pipeline)

**Files:** `accident_detector.h/.cpp` (no Arduino dependency), `sim_scenarios.h/.cpp` (14 SIMULATED scenarios)
**Document:** `Accident_Detection_Design.md` (stages, parameters with reasoning, limitations, pending items)
**Stages:** 1 candidate trigger (impact magnitude over a duration, optional jerk), 2 orientation change (gravity vector and gyro spike), 3 post-event stillness and GPS speed drop, 4 confirmation window.
**Tested with:** SIMULATED scenarios only: 42 checks in `Tests/Test_Suites/tests_accident.cpp`, results in `Tests/Scenario_Results.csv` and `Tests/Evidence/`. Replay tool: `Tests/Replay_Tool/`.
**Result on simulated data:** accident-like scenarios detected, normal driving, braking and cornering quiet, **2 known false positives** (bump followed by a hard stop; smooth-road bump with no GPS speed).
**Not validated:** all thresholds are placeholders. No real sensor, vehicle or crash data was used.
