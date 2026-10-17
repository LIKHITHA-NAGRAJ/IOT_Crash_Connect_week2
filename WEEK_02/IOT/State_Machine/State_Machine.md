# Device State Machine (IoT, Week 2)

> Tested in the Wokwi simulator with SIMULATED test inputs only. No real GNSS, IMU or modem was used.
> The diagram (`State_Machine_Diagram.png`) shows the main paths. The full list of allowed transitions is below.

**Code:** `Firmware/state_machine.h` and `Firmware/state_machine.cpp` (namespace `DeviceState`).

## States
BOOT, SELF_TEST, ACQUIRING_GPS, NETWORK_CONNECTING, OPERATIONAL, EVENT_ACTIVE, DEGRADED, FAULT

## Inputs (SIMULATED test conditions)
`selfTestOk`, `gpsFix`, `networkUp`, `eventDetected`, `eventCleared`.
The network condition is a simulated input; the comms stub itself always reports not connected.

## Rules applied by `DeviceState::step()`
| Current state | Condition | Next state |
|---|---|---|
| BOOT | always | SELF_TEST |
| SELF_TEST | selfTestOk | ACQUIRING_GPS |
| SELF_TEST | not selfTestOk | FAULT |
| ACQUIRING_GPS | gpsFix | NETWORK_CONNECTING |
| NETWORK_CONNECTING | networkUp | OPERATIONAL |
| OPERATIONAL | GPS fix lost or network down | DEGRADED |
| OPERATIONAL | eventDetected (and GPS and network OK) | EVENT_ACTIVE |
| EVENT_ACTIVE | eventCleared | OPERATIONAL |
| DEGRADED | gpsFix and networkUp | OPERATIONAL |
| FAULT | none | stays in FAULT |

## Allowed transitions (anything else is rejected)
- BOOT to SELF_TEST
- SELF_TEST to ACQUIRING_GPS or FAULT
- ACQUIRING_GPS to NETWORK_CONNECTING, DEGRADED or FAULT
- NETWORK_CONNECTING to OPERATIONAL, DEGRADED or FAULT
- OPERATIONAL to EVENT_ACTIVE, DEGRADED or FAULT
- EVENT_ACTIVE to OPERATIONAL, DEGRADED or FAULT
- DEGRADED to OPERATIONAL or FAULT
- FAULT to BOOT (reset)

`step()` only produces some of these (for example, nothing yet moves a state to DEGRADED from ACQUIRING_GPS or NETWORK_CONNECTING, because timeouts are not implemented). The allowed list is a draft.

## Simulated tests that exercised it (all PASS in simulation)
Normal start-up; movement burst (OPERATIONAL, EVENT_ACTIVE, OPERATIONAL); GPS lost and recovered (OPERATIONAL, DEGRADED, OPERATIONAL); self-test failure (SELF_TEST, FAULT) with FAULT to OPERATIONAL rejected.
See `Firmware/Test_Results/`.

## Not implemented
Timeouts, retry policy, recovery from FAULT, automatic periodic loop, fault codes.

## Pending Confirmation
- Wait for GPS first, or run GPS and network in parallel?
- Does a GPS or network timeout lead to DEGRADED or FAULT?
- What does FAULT do (retry, reset)? Can any state go straight to FAULT?
- Does EVENT_ACTIVE interrupt everything, or run alongside normal tracking?
