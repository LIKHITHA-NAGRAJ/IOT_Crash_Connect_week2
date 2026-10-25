# Test Results — Week 1 GPS + IMU Mini Demo

## Environment
- Simulator: Wokwi
- Board target: ESP32-S3
- Serial baud rate: 115200
- GPS input: recorded NMEA sentences
- IMU input: simulated values

## GPS test
- Parsed GGA and RMC example sentences.
- Example latitude: 48.117300
- Example longitude: 11.516667
- Example UTC time: 12:35:19
- Example recorded date: 23 March 1994
- Satellite count reported from GGA: 8
- HDOP reported from GGA: 0.90
- Checksum failures observed in the reported run: 0
- This does not verify live satellite reception or physical UART wiring.

## IMU tests
| Condition | AX (g) | AY (g) | AZ (g) | GX (deg/s) | GY (deg/s) | GZ (deg/s) | Magnitude (g) |
|---|---:|---:|---:|---:|---:|---:|---:|
| Stationary | 0.02 | 0.01 | 0.98 | 0.5 | 0.2 | 1.0 | 0.980 |
| Tilt/Rotation | 0.35 | 0.15 | 0.92 | 12.0 | 5.0 | 3.0 | 0.996 |
| Movement | 1.20 | 0.40 | 0.80 | 45.0 | 20.0 | 10.0 | 1.497 |

Formula: magnitude = sqrt(AX^2 + AY^2 + AZ^2).

## Status
The recorded-data GPS parser and simulated IMU calculations were observed in Wokwi. Physical sensor tests were not performed.
