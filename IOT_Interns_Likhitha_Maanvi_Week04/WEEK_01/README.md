# CRASH CONNECT - Week 1 IoT Deliverable

**Intern:** Likhitha N | **Role:** IoT | **Company:** CRASH CONNECT PVT LTD

> **GPS data = recorded NMEA sentences, NOT LIVE GPS.**
> **IMU data = SIMULATED / TEST DATA, not real sensor readings.**

## Project
GPS + IMU firmware mini demo on an ESP32-S3 (Wokwi simulation).
Flow: Input data -> ESP32 firmware -> Parse / Process -> GPS + IMU values -> Serial output

## Folder structure
| Folder | Contents |
|---|---|
| `firmware/` | `gps_imu_demo/gps_imu_demo.ino` - final GPS + IMU sketch |
| `source/` | Wokwi project files (`diagram.json`, `libraries.txt`) and run instructions |
| `sensor_data/` | recorded NMEA sentences, simulated IMU data (CSV) |
| `test_results/` | test report and serial output log |
| `screenshots/` | Wokwi Serial Monitor screenshots |
| `documentation/` | Day 1-5 documents and weekly summary |

## Development environment
- Wokwi ESP32-S3 simulator, Arduino C++ (Arduino IDE compatible)
- TinyGPSPlus library for NMEA parsing
- Serial Monitor at 115200 baud
- To run: see `source/README.txt`

## GPS implementation
- Recorded GGA and RMC sentences are fed into TinyGPS++, which checks the checksum and decodes the fields.
- Extracted: latitude, longitude (decimal degrees), UTC time/date, GPS fix status, satellites, HDOP, altitude.
- Clean location structure: device ID, latitude, longitude, timestamp, GPS status. A JSON payload is prepared only for a valid fix.
- Four recorded records are processed: two valid fixes, one no-fix, one corrupted sentence (rejected).
- Coordinates come from the parsed input, not hard-coded values.

## IMU implementation
- Six simulated values: AX, AY, AZ in g and GX, GY, GZ in deg/s.
- Three test cases (stationary, tilt/rotation, movement), five samples each, one sample every 500 ms.
- Acceleration magnitude = `sqrt(AX*AX + AY*AY + AZ*AZ)`.

## Tests performed
- Valid GGA + RMC parsing (two positions), no-fix handling, wrong-checksum rejection.
- Stationary, tilt/rotation and movement IMU cases; magnitudes 0.980 g, 0.996 g and 1.497 g for the first sample of each case.
- Details: `test_results/test_results.md` and `test_results/serial_output_log.txt`.

## What worked
- Recorded NMEA parsing and clean location output.
- No-fix and corrupted-sentence handling.
- Simulated IMU output with acceleration magnitude.
- Wokwi Serial Monitor runs of the Day 2 and Day 4 sketches.

## What could not be tested (no hardware available)
Live GPS reception, physical UART wiring, a real IMU, I2C communication, wireless transmission to the backend and real-world sensor accuracy.

## Problems and solutions
| Problem | Solution |
|---|---|
| No physical GPS/IMU hardware; no GPS module in Wokwi; Proteus needs a licence | Recorded NMEA sentences for GPS parsing and simulated values for IMU processing |
| TinyGPS++ read the two-digit year 94 as 2094 | Years 80-99 are mapped to 1980-1999 and 00-79 to 2000-2079 (general rule, no sample-specific workaround) |
| A no-fix sentence leaves the previous position stored in the library | GGA fix quality is read with `TinyGPSCustom` and checked before the location is reported |

## Known limitations
- No live GPS, physical UART, physical I2C or sensor accuracy tests.
- IMU values are fixed illustrative values, not real measurements or a physics simulation.
- No accident detection, filtering, calibration, wireless transmission or backend integration.
- The final sketch's Serial output was verified on a PC build; a Wokwi screenshot of the final sketch still has to be added (earlier sketches are shown in `screenshots/`).

## Week 2 recommendations and expectations
Recommendations:
1. Agree the JSON structure for location and motion data with the Full Stack team and send it to the FastAPI backend (HTTP POST over Wi-Fi).
2. Test GPS over UART (NEO-M8N) and an MPU6050 over I2C if hardware becomes available; otherwise keep using clearly labelled recorded/simulated inputs.
3. Split the code into GPS and IMU modules and add filtering of IMU noise.
4. Add a simple threshold check on acceleration magnitude as a first step towards impact detection.

Expectations:
- Move technical communication to Discord as announced.
- A clear task list and the agreed data format for the integration work.
- Hardware access, or confirmation that simulation/test data should continue.
