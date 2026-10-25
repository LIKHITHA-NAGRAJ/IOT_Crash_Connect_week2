# Device State Machine (IoT, Week 3 update, Wednesday)

> Tested with RECORDED GPS data and a MOCK modem only. No real GNSS, IMU or modem was used.
> `State_Machine_Diagram.png` is the Week 2 diagram (main paths). The rules below are current.

**Code:** `Firmware/state_machine.*` (namespace `DeviceState`) and `Firmware/device_integration.*` (builds the inputs).

## States
BOOT, SELF_TEST, ACQUIRING_GPS, NETWORK_CONNECTING, OPERATIONAL, EVENT_ACTIVE, DEGRADED, FAULT

## Inputs
| Input | Source |
|---|---|
| `selfTestOk` | self-test stub |
| `gpsFix` | `GNSS::getFix().valid` (current fix: fix flag, satellites, HDOP and age pass) |
| `networkUp` | MOCK modem reached `READY_SIMULATED` (not a real LTE connection) |
| `networkFailed` | MOCK modem is in a `FAILED_*` state |
| `eventDetected`, `eventCleared` | the staged accident detector (SIMULATED or recorded data), latched by `device_integration`; cleared after `eventHoldMs` |

## Rules applied by `DeviceState::step()`
| Current state | Condition | Next state |
|---|---|---|
| BOOT | always | SELF_TEST |
| SELF_TEST | selfTestOk / not | ACQUIRING_GPS / FAULT |
| ACQUIRING_GPS | gpsFix | NETWORK_CONNECTING |
| ACQUIRING_GPS | no fix for `gpsAcquireMs` | DEGRADED (policy PENDING) |
| NETWORK_CONNECTING | networkUp | OPERATIONAL |
| NETWORK_CONNECTING | networkFailed, or `networkConnectMs` exceeded | DEGRADED (policy PENDING) |
| OPERATIONAL | eventDetected (takes priority over the GPS and network checks) | EVENT_ACTIVE |
| OPERATIONAL | GPS lost or stale, or network down | DEGRADED |
| EVENT_ACTIVE | eventCleared | OPERATIONAL |
| DEGRADED | eventDetected | EVENT_ACTIVE |
| DEGRADED | gpsFix and networkUp | OPERATIONAL |
| FAULT | none | stays (reset only) |

## Allowed transitions (anything else is rejected)
BOOT to SELF_TEST; SELF_TEST to ACQUIRING_GPS or FAULT; ACQUIRING_GPS to NETWORK_CONNECTING, DEGRADED or FAULT; NETWORK_CONNECTING to OPERATIONAL, DEGRADED or FAULT; OPERATIONAL to EVENT_ACTIVE, DEGRADED or FAULT; EVENT_ACTIVE to OPERATIONAL, DEGRADED or FAULT; DEGRADED to OPERATIONAL, EVENT_ACTIVE or FAULT; FAULT to BOOT.

## What changed since Week 2
- Timeouts for GPS acquisition and network connection (placeholder limits in `config`).
- New `networkFailed` input; GPS and network now come from the real modules (recorded NMEA and mock modem), not hand-set booleans.
- Integration tests: normal start-up, GPS loss and recovery, stale fix, modem failure, GPS acquire timeout, self-test failure.

## Wednesday change
A confirmed event now takes priority over the GPS and network checks, and is accepted from DEGRADED as well as OPERATIONAL, because a crash may damage the GPS or network and the event must still be handled. Events that arrive during start-up states are dropped with a log message. Both are PENDING CONFIRMATION. See `Architecture/Accident_Detection_Design.md`.

## Thursday: events and the state machine
- SOS and accident events are created and queued in ANY state (including start-up), so none is lost. The state change is separate: EVENT_ACTIVE is entered only from OPERATIONAL or DEGRADED. If an event arrives during start-up the state is not changed, a message is logged, and the event is sent when the network is up.
- The manual SOS long press and the confirmed accident both use the same latch (`eventDetected` / `eventCleared`). EVENT_ACTIVE ends after the hold time, not when the event is acknowledged (PENDING).

## Not implemented
Retry/restart policy from DEGRADED, recovery from FAULT, periodic loop, fault codes.

## Pending Confirmation
See `Firmware/Design_Notes/Device_Status_Logic.md` and `Firmware/Design_Notes/Pending_Issues.md`.
