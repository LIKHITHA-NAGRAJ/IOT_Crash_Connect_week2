# Configuration Notes (Week 3, IoT)

> **ALL VALUES ARE PLACEHOLDERS** unless stated. None is a validated setting. Everything is defined in `Firmware/config.h` and `Firmware/config.cpp`, not scattered through the logic. "Pending" means it needs confirmation from the Technical Lead, Full Stack or PCB.

## General
| Setting | Value | Status |
|---|---|---|
| deviceId | `CC-IOT-01` | Format pending (the Full Stack sample uses `DEVICE-001`) |
| simulateHardware | true | Selects recorded, simulated and mocked inputs. Set false only when real drivers exist |
| intervals (gnss / imu / telemetry / status) | 1000 / 100 / 5000 / 2000 ms | Placeholders |
| timeouts: gpsAcquire / networkConnect / gnssStale / eventHold | 60000 / 30000 / 3000 / 3000 ms | Placeholders; DEGRADED versus FAULT on timeout is pending |
| comms: apn / serverHost / serverPort | `PENDING` / `PENDING` / 0 | Pending (no APN, endpoint or port is invented) |
| comms: maxRetries / retryDelayMs | 3 / 2000 | Placeholders (modem bring-up) |
| event: crashThresholdG (legacy Week 2) | 2.0 g | Used only by the legacy Week 2 telemetry test. The accident detector uses its own parameters below |
| pins (GNSS RX/TX, IMU SDA/SCL, modem RX/TX) | all -1 | Pending PCB. Proposal in `Firmware/Design_Notes/IoT_Interface_Table_and_Pin_Proposal.md` |

## GNSS and modem
| Setting | Value | Status |
|---|---|---|
| gnss: minSatellites / maxHdop | 4 / 5.0 | Placeholders |
| modem: powerUpMs / registerTimeoutMs / minRssi | 1000 / 15000 / 10 | Placeholders; AT syntax to verify against the A76XX manual |

## Accident detection (reasoning for each value: `Firmware/Accident_Detection/Accident_Detection_Design.md`)
| Setting | Value |
|---|---|
| impactThresholdG / impactMinMs | 3.0 g / 20 ms |
| jerkEnabled / jerkThresholdGps | false / 120 g/s |
| gravityTauMs / quietBandG | 1000 ms / 0.15 g |
| gyroSpikeDps / orientationChangeDeg / orientationSampleMs | 250 deg/s / 45 deg / 1000 ms |
| requireOrientation | false |
| settleMs / stillBandG / stillGyroDps / stillnessMinMs | 1000 ms / 0.08 g / 10 deg/s / 2000 ms |
| minPreSpeedKmh / speedDropDeltaKmh / postSpeedMaxKmh | 15 / 15 / 5 km/h |
| confirmWindowMs / cooldownMs | 6000 ms / 2000 ms |
Status: all placeholders chosen to exercise the software with SIMULATED data. Real values need logged vehicle data. The simulated vibration and noise levels behind some values are assumptions, not measurements.

## SOS and events
| Setting | Value | Status |
|---|---|---|
| sos: debounceMs / longPressMs / cooldownMs | 30 / 2500 / 5000 ms | Placeholders; the button, pin and circuit are pending PCB |
| events: schemaVersion | `1.0` | From the sample; added fields may need a new version |
| events: isoTimestamp | false (integer UTC epoch seconds) | Pending: the sample shows ISO text |
| events: maxLastKnownAgeS | 600 s | Placeholder; last-known location rule pending |
| events: retryBaseMs / retryMaxMs | 2000 / 60000 ms | Placeholders (exponential backoff) |
| queue capacity / payload size | 6 events / 480 bytes | Compile-time placeholders (`event_queue.h`); drop policy pending |

## Approved components (team list, 20 Oct)
ESP32-S3-WROOM-1-N8R2 (MCU), SIMCom A7670C (cellular), u-blox MAX-M10S-00B (GNSS), ST LSM6DSOX (6-axis motion sensor), TI LMR33630-Q1 (automotive buck), TI BQ24075 (battery management), 1200 mAh rechargeable lithium battery (backup). (Week 2 files listed a 3400 mAh 18650 battery; that was the earlier list.) No hardware was connected or tested.

## Open configuration questions
See `Firmware/Design_Notes/Pending_Issues.md`.
