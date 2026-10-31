# Week 1 IoT Documentation Summary

**Intern:** Likhitha N | **Role:** IoT | **Company:** CRASH CONNECT PVT LTD

## Daily work
| Day | Topic | Document |
|---|---|---|
| 1 | ESP32-S3 basics, Hello World, GPS/IMU sensor selection | Day1_Documentation.pdf |
| 2 | GPS to clean location data (recorded NMEA) | Day2_Documentation.pdf |
| 3 | NMEA GGA / RMC study | Day3_Documentation.pdf |
| 4 | IMU data simulation and processing | Day4_Documentation.pdf |
| 5 | GPS + IMU firmware mini demo | Day5_Documentation.pdf |

## Objective
Demonstrate GPS NMEA parsing and IMU data processing on an ESP32-S3 in the Wokwi simulator.

## GPS
Recorded GGA and RMC sentences are fed into the TinyGPS++ library. The firmware extracts latitude, longitude, UTC time/date, fix status, satellites, HDOP and altitude, prints a clean location summary and prepares a JSON payload for valid fixes. A no-fix record is flagged and a sentence with a wrong checksum is rejected.

## IMU
Simulated accelerometer (g) and gyroscope (deg/s) values are cycled through three test cases (stationary, tilt/rotation, movement), five samples each, one sample every 500 ms. Acceleration magnitude is calculated as sqrt(AX^2 + AY^2 + AZ^2).

## Firmware flow
Input data -> ESP32 firmware -> Parse / Process -> GPS + IMU values -> Serial output

## What worked
- Recorded NMEA parsing and clean location output.
- No-fix handling and checksum rejection.
- Simulated six-value IMU output with acceleration magnitude.
- Wokwi Serial Monitor demonstration of the earlier GPS and IMU sketches (see screenshots).

## What could not be tested (no hardware available)
Live GPS reception, physical UART wiring, a real IMU, I2C communication, wireless transmission to the backend and real-world sensor accuracy.

## Week 2
Test real UART/I2C interfaces if hardware is provided, define the JSON format with the Full Stack team and send the location data to the backend. See README.md for details.
