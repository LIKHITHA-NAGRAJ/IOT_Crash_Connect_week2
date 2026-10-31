# Email text and final report (copy and edit)

## Email body
Please find attached our Week 4 submission for the Standalone Vehicle Product phase.

Team: IoT

Included deliverables: Weeks 1-4 in one ZIP. Firmware v0.4 (GNSS handling, modem state machine with mocked AT, accident detector with replay tool, SOS long press, event builder with unique IDs, offline queue with retry and acknowledgement, device status message, adaptive reporting rate, error-code table, logging levels, simulated watchdog, no-GPS behaviour), test cases and results, configuration notes, README. A flat Wokwi project (`Wokwi_Project/`) with run instructions. Maanvi's offline-buffering, reconnect and latency simulations are included with her reports.

Testing completed: 103 unit tests passed on a PC; 11 accident-detection replay scenarios (10 as expected, 1 known false positive); 190 s integrated demo; 38 demo payloads checked against the flat format. All inputs were simulated, recorded (NMEA from Week 1) or mocked.

Known issues / pending items: not compiled or run on the ESP32-S3 or in Wokwi (the flat Wokwi project was only checked on a PC); GNSS, IMU, modem UART and SOS GPIO drivers and the `setup()/loop()` entry point are not written; queue is RAM only; AT command syntax and HTTPS/JWT unverified; accident thresholds are placeholders; contract mismatches with Full Stack (timestamp format, fix-quality type, event_id) to be confirmed; no real latency measurement; submitted after the 5:00 PM deadline.

All simulation, recorded-data and mock results are labelled accordingly. Physical hardware validation has not been performed where hardware was unavailable.

## Final report
✅ **Completed** (evidence in the ZIP): `Test_Results/` (unit output, demo log, replay output, contract check, sample messages); `Tests/Scenario_Results.csv`; `Error_Codes.md`; `Configuration_Notes.md`; `Offline_Buffering_Maanvi/outputs/`.

🧪 **Tested:** unit tests (SIMULATED/MOCK, 103 of 103); NMEA handling (RECORDED sentences); detector replay (SIMULATED synthetic, 10 of 11 as expected); modem transitions and retry/offline queue (MOCK); watchdog (SIMULATED forced hang); status, SOS and accident messages (SIMULATED).

⚠️ **Blockers:** no ESP32-S3 toolchain or Wokwi run in the workspace; no board, SIM or server access; pins, APN and server URL not confirmed; Full Stack contract answers pending; Maanvi's Week 3 IMU work and status checklist not received.

➡️ **Still open:** write the hardware drivers and entry point and run in Wokwi/on the board; flash-backed queue; verify AT syntax; JWT/HTTPS; real latency with Puneeth; tune detector with real data; joint review by Likhitha and Maanvi; add screenshots.
