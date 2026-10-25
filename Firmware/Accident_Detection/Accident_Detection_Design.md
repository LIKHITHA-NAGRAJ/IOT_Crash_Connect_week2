# Staged Accident Detection: Design (first implementation)

> **Status: software prototype tested on SIMULATED data only.** All thresholds are placeholders. Nothing here is validated accident-detection behaviour, and nothing was tested with a real sensor, vehicle or GPS.

## Pipeline
```
IMU samples (g, deg/s) ---> Stage 1: candidate trigger ----+
GPS speed (km/h, 1 Hz) ---> (pre-event speed captured)     |
                                                            v
                            Confirmation window (candidate + confirmWindowMs)
                              Stage 2: orientation change + gyro spike
                              Stage 3: stillness + GPS speed drop
                                                            v
                                  Stage 4: classify: CONFIRMED or REJECTED
                                                            v
              firmware: latch -> EVENT_ACTIVE (via device_integration)
```
Code: `Firmware/Accident_Detection/accident_detector.*` (no Arduino dependency, can be replayed on a PC), configuration in `config.*` (`AccidentConfig`), wiring in `device_integration.*`.

## Stages
1. **Candidate trigger.** The acceleration magnitude (g, includes gravity) must stay at or above `impactThresholdG` for at least `impactMinMs`. Optionally (`jerkEnabled`) a very fast magnitude change (`jerkThresholdGps`) also triggers. Jerk is off by default because it adds candidates from single-sample bumps.
2. **Orientation change.** A slow low-pass estimate of the gravity vector is kept while the signal is near 1 g. At the trigger it is frozen ("before"). In the last `orientationSampleMs` of the window, the mean acceleration is the gravity "after". The angle between them is compared with `orientationChangeDeg`. This is only evaluated when the device is quiet at the end. A **gyro spike** (peak angular rate in the settle period at or above `gyroSpikeDps`) is recorded as supporting evidence.
3. **Stillness and speed drop.** After `settleMs` (ignoring ringing), the signal is "quiet" when |magnitude - 1 g| <= `stillBandG` and angular rate <= `stillGyroDps`. Stillness needs `stillnessMinMs` of continuous quiet at the end of the window, and, if GPS speed is available, speed at or below `postSpeedMaxKmh`. A **speed drop** needs known pre- and post-event speed, pre-event speed >= `minPreSpeedKmh`, a drop of at least `speedDropDeltaKmh`, and a final speed <= `postSpeedMaxKmh`.
4. **Confirmation window.** The decision is made when `confirmWindowMs` has passed since the candidate. Rules, in order:
   - GPS speed known and the vehicle was not moving before the impact: **rejected** (parked or slow).
   - Neither stillness nor speed drop: **rejected** (the vehicle is still moving, or not still).
   - `requireOrientation` is on and there is no orientation change or gyro spike: **rejected**.
   - Otherwise **confirmed**. If no GPS speed was available it is marked **low confidence**.
After a decision, new candidates are ignored for `cooldownMs`.

## Parameters (all in `AccidentConfig`, none hard-coded in the logic)
| Parameter | Placeholder | Reason for the initial value | What is needed to set a real value |
|---|---|---|---|
| impactThresholdG | 3.0 g | Chosen above the simulated cornering and braking magnitudes (about 1.1 to 1.3 g) and below the simulated impacts (3.4 g and up). A starting point for software testing only | Logged normal-driving data and crash-test or real-event data |
| impactMinMs | 20 ms | Two consecutive samples at the assumed 50 Hz, so a single-sample spike is not a candidate | Real impact pulse durations and the real output data rate |
| jerkEnabled / jerkThresholdGps | off / 120 g/s | Jerk is shown as an option; off because single bumps trigger it | Real data |
| gravityTauMs / quietBandG | 1000 ms / 0.15 g | Slow estimate, updated only when near 1 g | Real mounting and motion data |
| gyroSpikeDps | 250 deg/s | Between the simulated tumble (400 deg/s) and simulated handling (45 deg/s) | Real gyro range and rotation data (LSM6DSOX range options to be checked in the datasheet) |
| orientationChangeDeg | 45 deg | Between the simulated rollover (90 deg) and no-rotation cases (about 0 deg) | Real mounting tolerance and rollover data |
| orientationSampleMs | 1000 ms | One second of quiet data to average | Real settling behaviour |
| requireOrientation | false | A frontal impact does not change orientation, so requiring it would miss it in simulation | Technical Lead policy |
| settleMs | 1000 ms | Ignore ringing right after the impact | Real data |
| stillBandG / stillGyroDps | 0.08 g / 10 deg/s | Above the simulated rest noise (0.01 g) and below the simulated driving vibration (0.12 g). **The vibration level is an assumption, not a measurement** | Real road-vibration and rest-noise data |
| stillnessMinMs | 2000 ms | Two seconds of quiet before deciding the vehicle has stopped | Real data and policy |
| minPreSpeedKmh / speedDropDeltaKmh / postSpeedMaxKmh | 15 / 15 / 5 km/h | Round placeholders: the vehicle must have been moving and must end up almost stopped | GPS speed accuracy and latency, policy |
| confirmWindowMs | 6000 ms | Long enough to see the stop and a second of stillness in simulation | Policy (response time versus false positives) |
| cooldownMs | 2000 ms | Avoid repeated triggers from one event | Policy |

## Firmware integration (this is the Likhitha part)
- `Integration::feedImu()` and `feedGpsSpeed()` / `feedSpeedFromGnss()` feed the detector. GNSS speed (knots) is converted to km/h; with no valid fix, speed is "unavailable".
- A confirmed event is latched. `Integration::applyEventLatch()` turns the latch into the state machine's `eventDetected` and `eventCleared` inputs (hold time = `timeouts.eventHoldMs`).
- **State machine changes:** a confirmed event now takes priority over the GPS and network checks, and is accepted from DEGRADED as well as OPERATIONAL (a crash may damage the GPS or network). Events during start-up states are dropped with a log message. Both are **PENDING CONFIRMATION**.
- Nothing is sent yet: there is no event message, SOS module or transmission. See `Firmware/Design_Notes/Pending_Issues.md`.

## Tools for working with data (for Manvi and offline analysis)
- `Tests/Replay_Tool/export_scenarios.cpp` writes the simulated scenarios to CSV (`Test_Data/Accident_Scenarios/`).
- `Tests/Replay_Tool/replay_csv.cpp` replays any CSV through the detector and lets you override parameters.
- CSV columns: `t_ms,ax_g,ay_g,az_g,gx_dps,gy_dps,gz_dps,speed_kmh` (speed: number = valid, `NA` = unavailable, empty = no GPS update at this row).
- Manvi's filtering, ring buffer, CSV export and rest-offset/gravity/axis-mapping work can plug in before the detector (`feedSample`) without changing it. Coordinate before editing `accident_detector.*` or `config.*`.

## Known limitations
- Only simulated data. No real sensor noise, mounting, saturation, GPS latency or real crash signatures.
- The assumed 50 Hz rate and the simulated vibration level may not match reality. The LSM6DSOX measurement range and output data rate options still need to be read from the datasheet (Manvi's task); high-g impacts may saturate the sensor.
- No filtering, no calibration of mounting orientation, no rest-offset estimate, no axis mapping yet.
- Orientation is only evaluated when the device is quiet at the end of the window.
- Two documented false-positive risks (see `Tests/Evidence/Accident_Scenario_Results.md`), and stillness cannot separate steady smooth cruising from being stopped when GPS speed is unavailable.
- A crash followed by loss of power or GPS before the window ends would not be classified (there is no early-escalation path).

## Pending Confirmation
1. Real thresholds and windows, and the data used to set them.
2. Whether orientation change must be required, and the policy for frontal-type impacts.
3. Handling of a hard stop after a bump (false-positive mitigation) and a driver cancel step.
4. Behaviour without GPS speed (confirm on IMU alone, or hold for more evidence).
5. Whether an event during start-up states should be handled.
6. Event message format and what the device does after confirmation (see `Firmware/Design_Notes/Pending_Issues.md`).
