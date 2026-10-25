# Week 3 Test Plan and Results (IoT)

> Inputs are RECORDED (GNSS NMEA), SIMULATED (IMU values, simulator time) or MOCKED (modem AT responses).
> A pass proves the software logic only. It does NOT validate real GNSS, IMU, modem or network hardware.
> `READY_SIMULATED` is not a real LTE registration, and a generated JSON packet is not a transmission.

**How to run:** Wokwi, ESP32-S3. Copy every `.h`, `.cpp` and `.ino` file from `Firmware/` and all its module folders, plus all files in `Tests/Test_Suites/`, into ONE flat Wokwi project (Wokwi has no folders), and replace `diagram.json` with `Firmware/diagram.json`. `sketch.ino` runs the suites in order and prints a summary.
The integration suite simulates about two minutes. Switch suites off with the `RUN_...` flags at the top of `sketch.ino`.

| Suite | File | Input | Checks | Result (from MY Wokwi run) |
|---|---|---|---|---|
| GNSS | `Test_Suites/tests_gnss.cpp` | RECORDED NMEA: valid, no fix, low satellites, high HDOP, bad checksum, stale fix, fix lost, loss and recovery, malformed lines | 31 expected | [fill in: x passed, y failed] |
| Modem | `Test_Suites/tests_modem.cpp` | MOCK AT responses: all OK, no response, no SIM, registration denied, registration timeout, weak signal, data failure, APN pending | 14 expected | [fill in] |
| Integration | `Test_Suites/tests_integration.cpp` | Recorded NMEA + MOCK modem: normal start-up, GPS loss and recovery, stale fix, modem failure, GPS acquire timeout, self-test failure | 8 expected | [fill in] |
| Accident detection | `Test_Suites/tests_accident.cpp` | SIMULATED scenarios (14) in two runs, config-change checks, detector to state machine, GNSS speed conversion | 42 expected | [fill in] |
| SOS and events | `Test_Suites/tests_events.cpp` | SIMULATED button signal, event payloads, mock transport and backend, simulated flash, GPS validity and state machine | 56 expected | [fill in] |
| Legacy telemetry | `Test_Suites/tests_telemetry_legacy.cpp` | Week 2 NESTED draft packet: valid, no fix, IMU unavailable, movement event, DEGRADED, FAULT | Manual review of output | [fill in notes] |

The expected counts come from a host-side build of the same code. Replace them with your own Wokwi results.

## Accident scenario results
See `Accident_Scenario_Results.md` and `../Scenario_Results.csv` (table of all 14 simulated scenarios with trigger counts and the two documented false positives). The same scenarios are available as CSV in `../../Sample_Data/` and can be replayed on a PC with `../../Tools/replay_csv.cpp`.

## Other files in this folder
- `Replay_Log_default_config.txt` and `Replay_Log_jerk_on.txt`: per-scenario decisions from replaying the 14 simulated CSV scenarios (`Test_Data/Accident_Scenarios/`) (PC build of the same detector code).
- `Accident_Scenario_Results.md`: scenario table with trigger counts and the two known false positives.
- `Event_Retry_Duplicate_Results.md`: retry, duplicate and offline results, and the SOS-to-dashboard status (not done).
- `Expected_Output_PC_Build.txt`: reference output from a PC build, for comparison only.

## Notes
- The legacy telemetry suite writes `timestamp_utc` as null for recorded data, so the 1994 sample time is never sent as current time. The packet is still the nested Week 2 draft, not the agreed flat schema.
- The GPS acquire timeout test waits for the placeholder 60 s limit in simulated time.

## Evidence
Paste the Serial Monitor output into `wokwi_serial_output_week3.txt` and add screenshots to `Screenshots/` (in this folder).

## Not tested
Real GNSS reception, real IMU readings, real modem and network, timing on hardware, flat-schema packet, API acceptance and rejection.
