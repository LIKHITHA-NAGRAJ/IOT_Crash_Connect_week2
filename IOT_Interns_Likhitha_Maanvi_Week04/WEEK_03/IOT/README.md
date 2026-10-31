# WEEK_03 / IOT - where the Week 3 work is

The Week 3 modules were integrated into one firmware project that was then extended in Week 4, so the code is NOT duplicated here. It is in `../../WEEK_04/IOT/`. Everything is SIMULATED, RECORDED or MOCK; nothing ran on the ESP32-S3.

| Week 3 item (Cathy's layout) | Location in WEEK_04/IOT |
|---|---|
| Firmware/GNSS | Firmware/GNSS (recorded NMEA tests, NO_FIX, GPS loss, last valid fix and age) |
| Firmware/IMU | Firmware/IMU (ring buffer, filter, CSV). Maanvi's own Week 3 IMU/logging work was NOT received; this was written by Likhitha |
| Firmware/Modem | Firmware/Modem and Firmware/Sim (state machine with MOCK AT responses; no real registration or LTE) |
| Firmware/Accident_Detection | Firmware/Accident_Detection (placeholder thresholds) |
| Firmware/SOS_Events | Firmware/SOS_Events (long press, debounce, event IDs, queue, retry/ACK) |
| Firmware/Telemetry | Firmware/Telemetry (flat telemetry and event builders) |
| Tests/Test_Cases.md, Scenario_Results.csv, Replay_Tool | Tests/ |
| Test_Data | Test_Data/ |
| Configuration_Notes.md, README.md | WEEK_04/IOT/ |

Incomplete or mock-only: hardware drivers (GNSS UART, LSM6DSOX, modem UART, SOS GPIO), real modem registration, flash-backed queue. See `../../WEEK_04/IOT/Contract_and_Pending_Issues.md`.
