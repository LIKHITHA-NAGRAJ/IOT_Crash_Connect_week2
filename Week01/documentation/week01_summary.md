# Week 1 IoT Documentation Summary

## Objective
Demonstrate GPS NMEA parsing and IMU data processing in an ESP32-S3 simulation.

## GPS
GGA and RMC sentences were used as recorded test inputs. TinyGPS++ extracted latitude, longitude, time/date and GPS-related fields. The sample output reported latitude 48.117300 and longitude 11.516667.

## IMU
Simulated accelerometer and gyroscope values were generated for stationary, tilt/rotation and movement. The acceleration magnitude was calculated from the three acceleration axes.

## Firmware flow
Input data -> ESP32 firmware -> Parse/process -> GPS and IMU values -> Serial Monitor.

## What worked
- Recorded NMEA parsing and clean location output.
- Simulated six-axis IMU output.
- Acceleration magnitude calculations.
- Wokwi Serial Monitor demonstration.

## What could not be tested
Physical GPS reception, live GPS data, physical UART wiring, physical IMU readings, I2C communication and real-world sensor accuracy were not tested because hardware was unavailable.

## Week 2
Continue improving validation and code organization. Test real UART/I2C interfaces if hardware becomes available. Prepare a clean data structure for future integration.
