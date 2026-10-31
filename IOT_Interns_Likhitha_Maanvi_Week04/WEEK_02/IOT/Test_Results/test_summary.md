# Test Summary - Week 2

**SIMULATED / TEST DATA.** Run on a PC, not on an ESP32-S3.

## How it was run
`firmware/host_tests/run_tests.sh` compiles the firmware modules in `firmware/crashconnect_fw/src/` with g++ together with the real TinyGPS++ source and a small stand-in for the Arduino functions (`Serial`, `millis`, `delay`). Two programs are built:
- `unit_tests`: checks individual behaviour.
- `scenario`: runs the real `crashconnect_fw.ino` (setup and loop) against the timetable of test inputs in `src/sim/`.

## Results
| Area | Checks | Result |
|---|---|---|
| State machine | every transition, timeouts, critical fault from every state | 29 passed |
| Fault flags | critical and non-critical | 4 passed |
| Safety event and IMU | threshold edge, SOS priority, invalid sample, magnitude formula | 8 passed |
| GNSS | valid fix, decoding, year rule, no fix, wrong checksum, stale data, GN talker | 12 passed |
| Telemetry | all values, unavailable values, missing inputs, buffer too small, sequence counter | 13 passed |
| **Total** | | **66 passed, 0 failed** |
| Demonstration | 85 s timetable, all 8 states visited | 20 packets sent to the stub, 1 not sent (network down), 1 corrupted sentence rejected |
| JSON check | every printed packet parsed with a JSON parser | 25 of 25 valid |

Files: `unit_test_output.txt`, `scenario_output.txt`.

## What this does not show
- No ESP32-S3 compile or run, no Wokwi run.
- No real GNSS, IMU, button or modem.
- The impact rule is a placeholder, not accident detection.
