# IoT Architecture Overview (Week 2)

> Simulated / recorded / test inputs only. Nothing here validates real hardware.

## Data flow
**Planned:** GNSS + IMU, then ESP32-S3 parsing, validation and event processing, then JSON telemetry, then A7670C LTE modem, then MQTT or REST API, then backend, database, dashboard.
**Implemented (simulated):** recorded GNSS input and test IMU values, then firmware and state handling, then JSON built locally. Not transmitted.

## Approved reference components (not connected)
ESP32-S3, SIMCom A7670C, u-blox MAX-M10S-00B, ST LSM6DSOX, TI LMR33630-Q1, TI BQ24075, 3400 mAh 18650 Li-ion backup battery.

## Firmware modules
| Module | Responsibility | Status |
|---|---|---|
| Configuration | Device ID, simulation flag, intervals, thresholds, timeouts, pins | Placeholder values; pins = -1 |
| GNSS | Parse GGA/RMC NMEA, check checksum, report fix | Recorded NMEA only; real UART not implemented |
| IMU | Accel/gyro and magnitude | Test values only |
| Communication | LTE modem link | Stub, never connects or sends |
| State machine | Device states | Implemented for simulated inputs |
| Telemetry | JSON packet | Draft builder with null handling |
| Event detection | Crash check | Placeholder threshold in the test sketch only |
| SOS, device status, fault codes | Planned in Monday list | NOT implemented |

## Interface assumptions (PENDING CONFIRMATION with PCB)
GNSS on UART; modem on UART; IMU on I2C or SPI (undecided); all pins unassigned (-1); logic voltage levels between the ESP32-S3 and the A7670C to be confirmed.

## Draft telemetry packet (PENDING CONFIRMATION)
```json
{"device_id":"CC-IOT-01","timestamp_utc":"1994-03-23T12:35:19Z",
 "location":{"valid":true,"latitude":48.117300,"longitude":11.516667},
 "imu":{"available":true,"accel_x_g":0.020,"accel_y_g":0.010,"accel_z_g":0.980,
        "gyro_x_dps":0.500,"gyro_y_dps":0.200,"gyro_z_dps":1.000},
 "event_active":false,"device_state":"OPERATIONAL","simulated":true}
```
- Unavailable values are `null`, never 0 or an invented number. No fix: `location.valid:false` with null latitude, longitude and timestamp. IMU unavailable: `imu.available:false` with six null values.
- Units: decimal degrees, g, degrees per second. The 1994 date comes from a public NMEA sample.
- Not in the packet yet: speed, GNSS fix quality, sequence number, network status, event message and event ID. Field names must be compared with the agreed structure and the Full Stack contract.

## Related files
`Monday module list and state diagram`: `IoT_Week2_Monday_Firmware_Architecture.pdf` (this folder).
`State machine details`: `../State_Machine/State_Machine.md`.
`Code and test evidence`: `../Firmware/`.
