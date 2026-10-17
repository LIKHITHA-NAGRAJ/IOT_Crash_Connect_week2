# Crash Connect - IoT / Embedded Week 2 Deliverable

**Intern:** Likhitha N | **Company:** Crash Connect Pvt Ltd
**Target:** ESP32-S3 | Arduino framework | Wokwi simulation | Serial Monitor 115200 baud

> **Simulation disclosure:** Everything here comes from simulated, recorded or test inputs. GNSS uses recorded NMEA sentences, IMU readings are test values, and the cellular link is a stub. A network-up condition appears only as a simulated test input. No physical GNSS, IMU, LTE modem or PCB was used. "Packet build: SUCCESS" means the JSON was created, not that it was sent.

## 1. Overview
A modular ESP32-S3 firmware skeleton run in Wokwi: separate modules, an eight-state device state machine, a draft JSON telemetry packet builder, and six simulated test scenarios.

## 2. Files
```
WEEK_02/IOT/
  Firmware/        Wokwi source files, Test_Results/, Screenshots/
  Architecture/    Architecture_Overview.md, Monday module list and state diagram (PDF)
  State_Machine/   State_Machine.md, State_Machine_Diagram.png
  README.md        this file
```
In Wokwi the firmware files are a flat list. The intended layout is one header and one source file per module.
Wokwi project name: ESP32_S3_final. The link is not included (work is under NDA).

**How to run:** create an ESP32-S3 Arduino project in Wokwi, add the files from `Firmware/`, press Start, and open the Serial Monitor. Six tests run once in `setup()`.

## 3. Week 2 work
| Day | Work | Status |
|---|---|---|
| Monday | Module list, device-state diagram, data-flow understanding | Documented in `Architecture/` |
| Tuesday | Project structure, module interfaces, configuration draft | Done; hardware-dependent parts are stubs |
| Wednesday | Eight-state model, transitions, telemetry builder | Done for simulated behaviour |
| Thursday | Testing, debugging, null handling, labelling | Fixes listed in section 8 |
| Friday | Consolidation, six simulated tests, README | Results in section 7 |

## 4. Architecture and data flow
**Planned:** GNSS + IMU, then ESP32-S3 parsing, validation and event processing, then JSON telemetry, then A7670C LTE modem, then MQTT or REST API, then backend, database, dashboard.
**Implemented here:** recorded GNSS input and simulated IMU input, then firmware and state handling, then JSON built locally. Transmission, backend receipt and hardware integration were not demonstrated.
**Approved parts (reference only, not connected):** ESP32-S3, SIMCom A7670C, u-blox MAX-M10S-00B, ST LSM6DSOX, TI LMR33630-Q1, TI BQ24075, 3400 mAh 18650 Li-ion backup battery.

## 5. Modules and status
| Module | Status |
|---|---|
| Configuration | Device ID, simulation flag and crash threshold are used by the sketch. Intervals, timeouts and pins (-1) are defined but not used yet; all are placeholders |
| GNSS | Parses recorded GGA/RMC with checksum check; reports fix or no fix. Real UART not implemented |
| IMU | Test values only (a stationary reading and a movement burst). No real sensor read |
| Communication | Stub: never connects, never sends |
| State machine | Eight states, transitions decided from test inputs and checked against an allowed list |
| Telemetry | Draft JSON builder with null handling |
| Event detection | Placeholder magnitude-threshold check in the test sketch only. Not a separate module |
| SOS | Not implemented |
| Device status / health | Not implemented |
| Fault handling | FAULT state exists and illegal transitions are rejected. No fault codes |

The Monday plan included SOS, device status and fault handling as separate modules. They were not built in Week 2.

## 6. State machine
States: BOOT, SELF_TEST, ACQUIRING_GPS, NETWORK_CONNECTING, OPERATIONAL, EVENT_ACTIVE, DEGRADED, FAULT.
Demonstrated transitions:
- BOOT, SELF_TEST, ACQUIRING_GPS, NETWORK_CONNECTING, OPERATIONAL (network-up was simulated)
- OPERATIONAL to EVENT_ACTIVE and back (test IMU values, placeholder threshold)
- OPERATIONAL to DEGRADED and back (simulated GPS loss and recovery)
- SELF_TEST to FAULT; FAULT to OPERATIONAL rejected (FAULT only goes to BOOT)

Not implemented: timeouts, retry policy, recovery from FAULT, automatic loop.

## 7. Draft telemetry packet (Pending Confirmation)
```json
{"device_id":"CC-IOT-01","timestamp_utc":"1994-03-23T12:35:19Z",
 "location":{"valid":true,"latitude":48.117300,"longitude":11.516667},
 "imu":{"available":true,"accel_x_g":0.020,"accel_y_g":0.010,"accel_z_g":0.980,
        "gyro_x_dps":0.500,"gyro_y_dps":0.200,"gyro_z_dps":1.000},
 "event_active":false,"device_state":"OPERATIONAL","simulated":true}
```
- Unavailable values are JSON `null`, never 0 or an invented number. No GPS fix gives `location.valid:false` with null latitude, longitude and timestamp. An unavailable IMU gives `imu.available:false` with six null values.
- Location and IMU are handled independently.
- Units: decimal degrees, acceleration in g, angular velocity in degrees per second.
- The 1994 timestamp comes from the public NMEA sample, not live GPS time.
- `simulated` is an extra labelling field; whether it belongs in the real packet is not agreed.
- Comparison with the Full Stack API contract: not completed in this document (see section 10).

## 8. Test results (Wokwi, simulated)
Evidence: `Firmware/Test_Results/wokwi_serial_output.txt` and `Firmware/Screenshots/`.
| Test | Result |
|---|---|
| 1. Normal start-up | BOOT to OPERATIONAL; packet built, not sent. PASS (simulated) |
| 2. No GPS fix | Timestamp, latitude, longitude null; validity false. PASS (simulated) |
| 3. IMU unavailable | Six IMU values null; availability false. PASS (simulated) |
| 4. Movement burst | 2.474 g above the 2.00 g placeholder; EVENT_ACTIVE then OPERATIONAL. PASS (simulated) |
| 5. GPS lost and recovered | OPERATIONAL to DEGRADED to OPERATIONAL. PASS (simulated) |
| 6. Self-test failure | SELF_TEST to FAULT; illegal FAULT to OPERATIONAL rejected. PASS (simulated) |

Build status: the sketch compiled and ran in Wokwi. A PlatformIO build for the ESP32-S3 target was not performed for this firmware. Test 2 shows state OPERATIONAL because the state was left over from Test 1.

## 9. Problems found and fixed
- Sketch called GNSS functions that no longer existed after the GNSS module was rewritten (compile error); the sketch was updated.
- Coordinates were hard-coded in the sketch; they now come only from parsed recorded NMEA.
- Timestamp was always written as null and a real one would have been unquoted; fixed.
- Valid data was lost when only one sensor was unavailable; location and IMU blocks are now built separately.
- Latitude and longitude were `float`; changed to `double`.
- States could be set to any other state; an allowed-transitions check was added.

## 10. Known limitations
1. No physical testing: GNSS, IMU, modem, PCB, power switching and the backup battery are unverified.
2. No transmission: the modem is a stub and packets were not sent to any API or MQTT broker.
3. No validated accident detection: the 2.00 g threshold is a placeholder.
4. No periodic loop, timeouts, retries or recovery policy; the six tests run once in `setup()`.
5. SOS, device status and fault codes are not built.
6. Packet field names, null handling and the event message are not confirmed with Full Stack.
7. The too-small-buffer case of the packet builder was not tested in Wokwi.

## 11. Pending Confirmation
1. Final packet field names, types, units, nesting, the `simulated` field, and API acceptance of `null`.
2. Whether packets without a GPS fix are sent; send rate; buffering and resend.
3. Event/SOS message format and backend acknowledgement.
4. GPS and network acquisition: sequential or parallel.
5. Timeouts, retry policy, DEGRADED versus FAULT behaviour, reset and recovery.
6. MQTT versus REST API; A7670C UART/AT interface.
7. IMU I2C versus SPI; validated event criteria; manual SOS input.
8. ESP32-S3 board target, pin mapping, PCB wiring, power and backup switching.
9. Full Stack confirmation that this packet matches the API contract.

## 12. Week 3 next steps
- Agree and freeze the telemetry and event messages with Full Stack.
- Confirm pin mapping and interfaces with the PCB team.
- Add periodic scheduling, timeouts, retry and fault codes.
- Build event, SOS and device-status modules with explicit tests.
- Test real GNSS, IMU and modem drivers when hardware is available.
- Keep the build output and Git commit history for each step.

**Git history:** repository `LIKHITHA-NAGRAJ/Crash_connect`, branch `esp32_s3`. [add commit summary from `git log --oneline` after pushing]
