# Test Results - Week 1 GPS + IMU Mini Demo

## Environment
- Target board: ESP32-S3 (Wokwi simulator, Arduino C++)
- Library: TinyGPSPlus
- Serial baud rate: 115200
- GPS input: recorded NMEA sentences (NOT LIVE GPS)
- IMU input: simulated / test values (NOT real sensor readings)

## How these results were obtained
- The Wokwi screenshots in `screenshots/` come from the earlier Day 2 (GPS) and Day 4 (IMU) sketches.
- `serial_output_log.txt` is the output of the final combined sketch (`firmware/gps_imu_demo/gps_imu_demo.ino`), checked on a PC build with the real TinyGPSPlus library and an Arduino stub, because the final sketch was updated after the Wokwi runs. Re-run it in Wokwi and add a screenshot of the final output to `screenshots/`.

## GPS tests (recorded NMEA data)
| # | Input | Expected | Result |
|---|---|---|---|
| 1 | Valid GGA + RMC (123519) | Lat 48.117300, Lon 11.516667, 1994-03-23T12:35:19Z, fix 1, 8 satellites, HDOP 0.90 | Pass |
| 2 | Valid GGA + RMC (123520) | Lat 48.117417, Lon 11.516867, 12:35:20 | Pass |
| 3 | No fix (GGA fix quality 0, RMC status V) | Status NO FIX, lat/lon INVALID, no payload prepared | Pass |
| 4 | GGA with wrong checksum | Sentence rejected, checksum failures = 1 | Pass |

This does not verify live satellite reception or physical UART wiring.

## IMU tests (simulated / test data)
Formula: magnitude = sqrt(AX^2 + AY^2 + AZ^2). Each test case prints 5 samples, one every 500 ms; the first sample of each case is shown here and all 15 are in `sensor_data/imu_simulated_test_data.csv`.

| Condition | AX (g) | AY (g) | AZ (g) | GX (deg/s) | GY (deg/s) | GZ (deg/s) | Magnitude (g) |
|---|---:|---:|---:|---:|---:|---:|---:|
| Stationary | 0.02 | 0.01 | 0.98 | 0.5 | 0.2 | 1.0 | 0.980 |
| Tilt/Rotation | 0.35 | 0.15 | 0.92 | 12.0 | 5.0 | 3.0 | 0.996 |
| Movement | 1.20 | 0.40 | 0.80 | 45.0 | 20.0 | 10.0 | 1.497 |

Magnitude range per case: stationary 0.980-1.000 g, tilt/rotation 0.996-1.008 g, movement 1.378-3.454 g. All 15 magnitudes were re-calculated separately and match.

## Observations
- At rest the magnitude stays near 1 g (gravity), with only small gyroscope values.
- Tilting shifts gravity between the axes and the gyroscope rises, but the magnitude stays near 1 g.
- Movement produces large acceleration on several axes and large gyroscope values, so the magnitude rises well above 1 g.

## Status
GPS parsing and IMU calculations work on recorded/simulated data. Physical sensor tests were not performed because hardware was unavailable.
