# CRASH CONNECT - IoT Week 4 submission (Likhitha N, Maanvi Mishra)
Company-internal. Submitted after the 31 Oct 5:00 PM deadline.

**Everything here is SIMULATED, RECORDED or MOCK.** GNSS = recorded NMEA (Week 1) or generated NMEA with checksums; IMU, button, network and server = test inputs. The code was built and tested on a PC with g++ and the real TinyGPS++ source. **It has not been compiled for the ESP32-S3 or run in Wokwi, and no hardware was used.**

## Folder map (this folder = WEEK_04/IOT)
| Path | Contents |
|---|---|
| `Wokwi_Project/` | **FLAT project to run in Wokwi** (all files in one folder, `cc_` prefix). Start with `Wokwi_How_To_Run.md` |
| `Wokwi_Template/`, `tools/` | template files and the script that generates `Wokwi_Project/` from `Firmware/` |
| `Firmware/` | v0.4 modules: `GNSS`, `IMU`, `Modem`, `Accident_Detection`, `SOS_Events`, `Telemetry`, plus `State_Machine`, `Config`, `Storage`, `Device`, `Common`, `App`, `Sim` |
| `Tests/` | `Test_Cases.md`, `Scenario_Results.csv`, `Replay_Tool/`, `Unit/`, `Demo/`, build scripts, `validate_contract.py` |
| `Test_Data/` | synthetic scenario CSVs and `generate_scenarios.py` |
| `Test_Results/` | unit output, demo log, replay output, contract check, dead-code check, sample messages |
| `Offline_Buffering_Maanvi/` | Maanvi's Python simulations, reports, and re-run outputs |
| `Configuration_Notes.md`, `Error_Codes.md`, `Modem_Data_Approach.md`, `Contract_and_Pending_Issues.md`, `Integration_Notes.md`, `Status_Field_Checklist.md`, `Email_and_Final_Report.md` | documents |

## Flat format
Telemetry, status and event messages are flat JSON (one level, no nested objects), checked by `Tests/validate_contract.py`. The Wokwi project folder is flat as well.

## Run the tests (needs g++ and the TinyGPSPlus source)
```
export TINYGPS_SRC=<path to TinyGPSPlus/src>
Tests/build_demo.sh Tests/Unit/unit_tests.cpp Tests/Demo/demo_main.cpp
Tests/build/unit_tests
Tests/build/demo_main
Tests/build_replay.sh
Tests/wokwi_host_check.sh
```

## What is implemented (and how it was tested)
| Area | Status |
|---|---|
| GNSS: GP/GN talkers, checksum, NO_DATA / NO_FIX / FIX / STALE, last valid fix and age, UTC only from plausible dates | implemented, tested (recorded NMEA) |
| Modem state machine, mock AT, backoff 1-30 s, no-network and no-server handling | implemented, MOCK-tested; real UART port NOT written |
| IMU ring buffer, filter, CSV logging/replay | implemented, tested; LSM6DSOX driver NOT written |
| Accident detector (trigger, observation, stillness, rotation, GPS speed check) and replay tool | implemented, tested on synthetic data; thresholds are placeholders |
| SOS long press with debounce | implemented, tested; GPIO read NOT written |
| Event builder, unique IDs, location with age or clear no-fix | implemented, tested |
| Queues, retry, ACK, same event id, offline then online | implemented, MOCK-tested; RAM only, not flash-backed |
| Device ID and settings in non-volatile storage, status message on connect | implemented; ESP32 NVS path untested, PC uses a stand-in |
| Reporting interval and adaptive rate | implemented, tested |
| Error-code table, log levels, dead-code check | done (`Error_Codes.md`, `Test_Results/dead_code_check.txt`) |
| Watchdog | simulated forced hang tested; ESP32 task-watchdog code untested |
| `setup()/loop()` entry point for the board | NOT written (`app_setup`/`app_loop` are the hooks) |

Results: 103/103 unit tests, 10 of 11 replay scenarios as expected (one known false positive recorded as FAIL), 38 demo payloads valid against the flat format. See `Tests/Test_Cases.md`.

`WEEK_02` contains the old nested draft packet from before the flat format was agreed; it is superseded by this folder.

Weeks 1 and 2 are in `../../WEEK_01` and `../../WEEK_02`. Week 2 documents mention the old 3400 mAh battery; the approved cell is now 1200 mAh. Open issues: `Contract_and_Pending_Issues.md`.
