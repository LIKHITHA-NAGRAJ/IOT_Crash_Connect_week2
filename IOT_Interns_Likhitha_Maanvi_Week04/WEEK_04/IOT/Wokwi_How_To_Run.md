# How to run the firmware in Wokwi

**Honest status:** the `Wokwi_Project/` folder has NOT been run in Wokwi or compiled with the ESP32 toolchain by me (my workspace has neither). The same files, flattened, were compiled and run on a PC with g++ and an Arduino stand-in (`Test_Results/wokwi_sketch_host_check.txt`: sketch ran to the end, 38 payloads valid, 0 problems). Expect to fix small ESP32-only compile messages if any appear, and tell me what they say.

## What runs
`sketch.ino` runs a 190 s SIMULATED demo: generated NMEA (with checksums), synthetic IMU motion, a MOCK modem answering AT commands and a MOCK server. Nothing real is connected. The only real input is the optional SOS button on GPIO10.

## Steps (wokwi.com, browser)
1. Go to wokwi.com, New Project, choose **ESP32-S3**, Arduino.
2. Delete the default `sketch.ino`. Upload ALL files from `Wokwi_Project/` (every `cc_*.h`, `cc_*.cpp`, `sketch.ino`, `diagram.json`, `libraries.txt`). The project is flat on purpose: no sub-folders.
3. `libraries.txt` contains `TinyGPSPlus`. If Wokwi does not pick it up, add TinyGPSPlus (Mikal Hart) in the Library Manager.
4. Press Start and open the Serial Monitor (115200 baud). The demo starts about 1 s after boot.
5. Optional: press and hold the red SOS button for 2 s. That raises a real SOS event on top of the simulated ones.

## What to look for (times are seconds after start)
| Time | Log shows |
|---|---|
| 0-14 | boot, self-test, GPS acquiring, `[STATUS] connected` and ACK at about 11 s |
| 14 | fix acquired, OPERATIONAL, `[RATE] ... interval=30s` |
| 50 | stored stationary interval changed to 10 s |
| 70 | MOVING, interval 5 s |
| 80-87 | short SOS press cancelled, long press raises an SOS event |
| 88-100 | network removed, accident detected at about 97 s and queued, delivered after reconnect |
| 104-124 | server down, retries with the same event id |
| 125-145 | no GPS fix, null coordinates, SOS with `LAST_KNOWN` location and its age |
| 150-158 | forced hang, simulated watchdog reset, `reset_reason=TASK_WDT` |
| 190 | `DEMO SUMMARY` and the mock server's list of received event ids |

Real timing may differ from the PC log because Wokwi runs the sketch in its own time.

## Notes
- Watchdog: by default the reset is SIMULATED (`CC_USE_HW_WATCHDOG` is 0 in `cc_device.cpp`). Setting it to 1 enables the ESP32 task watchdog for real; the forced hang then causes a genuine restart and the demo starts over. That path is untested.
- The queues and settings live in RAM and the ESP32 `Preferences` area; a Wokwi restart may clear them.
- To use the structured project (Cathy's folder layout) instead, use `Firmware/`; the flat project is generated from it by `tools/make_wokwi_project.py`, so they cannot differ.
- No LSM6DSOX, MAX-M10S or A7670C is simulated. Wokwi has no matching parts, so those drivers are not part of this project.
