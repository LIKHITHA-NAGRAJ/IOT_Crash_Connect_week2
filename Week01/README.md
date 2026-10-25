# Week 1 IoT Deliverable — Likhitha N

## Project
GPS + IMU Firmware Mini Demo using ESP32-S3 in Wokwi.

## Development environment
- Wokwi ESP32-S3 simulator
- Arduino C++
- TinyGPSPlus library for NMEA parsing
- Serial Monitor at 115200 baud

## GPS implementation
The firmware feeds recorded GGA and RMC NMEA sentences into TinyGPS++. It extracts latitude, longitude, time/date, satellite count and HDOP, then prints a clean location summary.

**GPS input is recorded test data — NOT LIVE GPS.** The test does not prove live satellite reception or physical UART communication.

## IMU implementation
The firmware generates six simulated values: AX, AY, AZ in g and GX, GY, GZ in degrees per second. It cycles through stationary, tilt/rotation and movement test cases. Acceleration magnitude is calculated as:

`sqrt(AX*AX + AY*AY + AZ*AZ)`

**IMU readings are SIMULATED / TEST DATA.** No physical IMU was connected.

## Tests performed
- Parsed recorded GGA and RMC sentences using TinyGPS++.
- Checked extracted coordinates, UTC time/date, satellite count and HDOP in Serial Monitor.
- Observed zero checksum failures in the reported test output.
- Ran stationary, tilt/rotation and movement IMU test cases.
- Verified acceleration magnitude values: 0.980 g, 0.996 g and 1.497 g.

See `test_results/test_results.md` and `sensor_data/recorded_nmea_and_simulated_imu.txt`.

## Problems and solutions
- **Problem:** No physical GPS/IMU hardware; GPS component was not available or easy to locate in the simulator.
- **Solution:** Used recorded NMEA sentences to test parsing and simulated IMU values to test processing logic.
- **Problem:** GPS date formatting showed an unexpected century for the two-digit year in the recorded sample.
- **Solution:** The demo applies a correction for this specific known 1994 sample. This is only a test-specific workaround, not a general date conversion method.

## Known limitations
- No live GPS reception, physical UART wiring, physical I2C communication or sensor accuracy tests were performed.
- IMU readings are fixed illustrative values, not measurements from a real sensor or a physics simulation.
- No accident detection, filtering, calibration, wireless transmission or backend integration is implemented.
- The GPS timestamp correction is specific to the recorded sample.

## Week 2 recommendations and expectations
1. Refactor the GPS and IMU code into clear modules.
2. Improve NMEA validation and handle invalid/missing fields safely.
3. Review the receiver's date/year interpretation and remove sample-specific date workarounds.
4. If hardware becomes available, test GPS over UART and an MPU6050 over I2C.
5. If hardware remains unavailable, continue with clearly labelled recorded/simulated inputs.
6. Define a clean JSON structure for location and motion data before attempting backend transmission.
7. Keep screenshots and test results aligned with the actual run.

## Folder contents
- `firmware/`: demo firmware
- `source/`: source file and library note
- `sensor_data/`: recorded NMEA and simulated IMU data
- `test_results/`: test report
- `screenshots/`: available Wokwi screenshots
- `documentation/`: Week 1 summary


## Day 2 documentation
See `documentation/day2_gps_nmea_documentation.md` for the GPS-to-clean-location-data task, NMEA GGA/RMC meanings, recorded input, parsed fields, limitations and next steps.
