# Day 2 Documentation — GPS to Clean Location Data

## Objective
Understand the data flow from a GPS/GNSS source to the ESP32 firmware, parse recorded NMEA sentences, validate the parsed position, and print clean location fields.

## Development approach
Wokwi ESP32-S3 simulation was used because physical hardware and a suitable GPS simulator component were unavailable. Recorded NMEA sentences were used as input. This is a parsing exercise, not a live GPS test.

## NMEA sentence types
- **GGA** provides UTC time, latitude, longitude, fix quality, satellite count, HDOP and altitude-related fields.
- **RMC** provides UTC time, validity status, latitude, longitude, speed, course and date.

Example recorded inputs:
- GGA: `$GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47`
- RMC: `$GPRMC,123519,A,4807.038,N,01131.000,E,022.4,084.4,230394,003.1,W*6A`

## Parsing result
- Latitude: 48.117300°
- Longitude: 11.516667°
- UTC time: 12:35:19
- Recorded date: 23 March 1994
- Fix quality in GGA: 1 (GPS fix)
- Satellites: 8
- HDOP: 0.90

The values above belong to the recorded sample. They do not represent the current location or a live satellite fix.

## Firmware flow
Recorded NMEA input → ESP32 program → TinyGPS++ parsing → check parsed location validity → print clean location fields to Serial Monitor.

## What worked
The parser extracted the sample coordinates, time/date and GPS-related fields. The Serial Monitor displayed a clean location summary.

## Problems and solutions
The required GPS component was not available/easy to locate in the simulator, and Proteus required a licence. Recorded NMEA sentences were therefore used to test the parsing logic. The sample's two-digit year also required careful interpretation; any correction applied in the demo is specific to this recorded sample.

## Limitations
No live GPS reception, physical UART wiring, GPS antenna performance or real-world accuracy was tested. Recorded NMEA data only verifies the software parsing path.

## Next steps
Continue IMU data processing with clearly labelled simulated values, collect Serial Monitor screenshots, and keep the recorded-data limitation explicit in the README.
