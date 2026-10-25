# IoT Architecture Overview (Week 3, updated)

> Recorded / simulated / mocked inputs only. Nothing here validates real hardware.
> The Week 2 version of this file is kept unchanged in the Week 2 submission (`WEEK_02/IOT/Architecture/`).

## Approved components (as updated by the team on 20 Oct)
| Block | Part |
|---|---|
| MCU | ESP32-S3-WROOM-1-N8R2 module |
| Cellular | SIMCom A7670C |
| GNSS | u-blox MAX-M10S-00B |
| Motion sensor | ST LSM6DSOX (6-axis) |
| Automotive buck | TI LMR33630-Q1 |
| Battery management | TI BQ24075 |
| Backup power | 1200 mAh rechargeable lithium battery |

(The Week 2 files mention a 3400 mAh 18650 battery. That was the earlier list.)

## Data flow
**Planned:** GNSS + IMU, then ESP32-S3 parsing, validation, event handling and state machine, then telemetry, then A7670C modem, then MQTT or REST to the backend, database, dashboard.
**Implemented (simulated):** recorded GNSS input and mock modem drive the device state machine. The telemetry packet is built locally in the old nested draft format. Nothing is transmitted.

## Firmware modules
| Module | Responsibility | Status |
|---|---|---|
| Configuration (`config`) | Device ID, simulation flag, intervals, thresholds, timeouts, GNSS and modem limits, pins | Placeholder values; pins = -1 |
| GNSS (`gnss`) | Parse GGA/RMC; fix quality from GGA; satellites and HDOP gating; speed, course, UTC epoch seconds; fix age; last-valid tracking; NO_FIX | Recorded NMEA only; real UART NOT implemented |
| IMU (`imu`) | Accel and gyro test values and magnitude | Test values only |
| Modem (`modem`) | Mock AT-command bring-up: power-up, AT, SIM, registration, signal, APN, data session; retries, timeouts, failure states | MOCK only; real UART transport NOT implemented |
| Communication (`comms`) | Old LTE stub | Stub |
| Integration (`device_integration`) | Only place that turns GNSS and modem results into state-machine inputs | Done |
| State machine (`state_machine`) | 8 states, allowed-transition check, timeouts | Done for simulated inputs |
| Telemetry (`telemetry`) | JSON packet | Old NESTED Week 2 draft; flat schema NOT yet applied |
| Accident detector (`accident_detector`) | Staged pipeline: candidate trigger, orientation check, stillness and GPS speed drop, confirmation window | First implementation; SIMULATED data only; placeholder thresholds |
| SOS button (`sos_button`) | Debounce, long-press timing, feedback, cooldown | Simulated signal only |
| Events (`event_builder`, `event_manager`) | Builds SOS and accident events with event ID, time, location and age | Simulated; contract fields unconfirmed |
| Queue and transport (`event_queue`, `event_transport`) | Offline queue (simulated flash), retry with backoff, acknowledgement handling, mock backend | Simulated only |
| Simulated scenarios (`sim_scenarios`) | 14 generated driving and impact scenarios for replay | Simulated data only |
| SOS, device status, fault codes | Planned | NOT implemented |

## Interface assumptions (PENDING CONFIRMATION with PCB)
See `IoT_Interface_Table_and_Pin_Proposal.md`. All pins are a proposal; firmware pins are -1 until PCB confirms.

## Telemetry
The builder still produces the Week 2 nested draft, where `timestamp_utc` is null for recorded data and unavailable values are `null`. The agreed flat schema and integer epoch `timestamp_utc` are not applied yet, because the exact schema was not available. See `Flat_Packet_Compatibility_Notes.md`.

## Related documents (paths from the IOT folder)
`Firmware/SOS_Events/Event_Model_and_Transmission.md`, `Firmware/SOS_Events/Event_API_Test_Cases.md`, `Firmware/Accident_Detection/Accident_Detection_Design.md`, `Firmware/Modem/LTE_Bringup_Checklist.md`, `Firmware/Modem/Modem_Data_Approach.md`, `Firmware/Design_Notes/Device_Status_Logic.md`, `Firmware/Telemetry/Flat_Packet_Compatibility_Notes.md`, `Firmware/Design_Notes/IoT_Interface_Table_and_Pin_Proposal.md`, `Firmware/Design_Notes/Pending_Issues.md`, `Firmware/Design_Notes/State_Machine.md`.
