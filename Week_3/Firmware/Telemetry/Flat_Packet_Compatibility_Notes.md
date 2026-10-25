# Flat Telemetry Packet: Compatibility Notes (Week 3, Monday)

> I do not have the agreed flat packet definition yet. These notes list what is known, what is missing, and what must change. **No field names below are final.**

## Current state of the code
The Week 2 packet builder produces a **nested** structure (`location`, `imu` objects). The Week 3 instruction is to use the agreed Week 2 **flat** packet and not change it independently. So the builder is **not yet compliant** and must be re-aligned once the exact flat schema is in hand. Nothing was changed to the builder today.

## Field names mentioned in the team cross-check (exact spelling must be confirmed)
`device_id`, `timestamp_utc`, `latitude`, `longitude`, `speed`, `gnss_fix_quality`, `sequence_number`, `vehicle_status`, `accident_status`, `network_status`, plus acceleration in g and angular rate in deg/s.

## What the firmware can already supply
| Item | Source | State |
|---|---|---|
| device_id | config | available |
| timestamp_utc | integer UTC epoch seconds from the parsed GNSS sentence | computed; **recorded data is flagged not live and must not be sent as current time**; time source when there is no fix is PENDING |
| latitude, longitude | GNSS | available when the fix is valid; **null** otherwise |
| speed | GNSS speed over ground in knots; null without a fix | **unit PENDING** (knots, km/h or m/s) |
| course | GNSS course over ground, degrees | available; **not known whether the packet has it** |
| gnss_fix_quality | derived from the GGA fix flag; 0 when no valid fix | available |
| sequence_number | not implemented | **missing** |
| acceleration (g), angular rate (deg/s) | IMU test values only | available in the right units; flat field names unknown |
| network_status | modem state machine (mock) | mapping of modem states to status values **PENDING** |
| vehicle_status, accident_status | state machine / placeholder event logic | value sets and mapping **PENDING** |
| event message and event ID | not implemented | **missing** |

## Rules already followed
- Unavailable values are `null`, never 0 or invented.
- Acceleration in g, gyroscope in degrees per second.
- Accident thresholds are placeholders; simulated motion values test the software only.

## Questions for Full Stack and the Technical Lead
1. The exact flat field list, types and units (especially speed and the acceleration and gyro field names).
2. `timestamp_utc`: integer epoch seconds confirmed. What should the device send when it has no valid time?
3. Does the API accept `null` for latitude, longitude and speed, and what validation applies?
4. Allowed values for `vehicle_status`, `accident_status` and `network_status`, and how modem states map to `network_status`.
5. Event message fields and event ID handling.
6. Is course part of the packet?
7. Are no-fix packets sent, and at what rate?

## Planned change once confirmed
Rewrite `telemetry.cpp` to emit the flat schema exactly, test it with the same null cases, and record the results with the data source labelled (recorded GNSS, simulated sensor values, mock modem).
