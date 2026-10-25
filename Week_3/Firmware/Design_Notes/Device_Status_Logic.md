# How GPS and Network States Affect Device Status

> Tested with RECORDED GPS data and a MOCK modem only. "Network up" means the mock modem reached
> `READY_SIMULATED`. It is NOT a real LTE registration or data connection.

## Where it happens
`device_integration.cpp` is the only place that reads the GNSS fix and the modem status and builds the state-machine inputs. `gnss.*`, `modem.*` and `state_machine.*` do not depend on each other.

| Input to the state machine | Comes from |
|---|---|
| `gpsFix` | `GNSS::getFix().valid` (a CURRENT fix: fix flag, satellites, HDOP and age all pass) |
| `networkUp` | modem state is `READY_SIMULATED` (mock) |
| `networkFailed` | modem is in any `FAILED_*` state |
| `selfTestOk`, `eventDetected`, `eventCleared` | self-test stub, placeholder event logic |

## Effect on device state
| Current state | GPS | Network | Next state |
|---|---|---|---|
| ACQUIRING_GPS | valid fix | any | NETWORK_CONNECTING |
| ACQUIRING_GPS | no fix for `gpsAcquireMs` | any | DEGRADED (policy PENDING) |
| NETWORK_CONNECTING | any | up | OPERATIONAL |
| NETWORK_CONNECTING | any | failed, or `networkConnectMs` exceeded | DEGRADED (policy PENDING) |
| OPERATIONAL | lost or stale | any | DEGRADED |
| OPERATIONAL | valid | down | DEGRADED |
| DEGRADED | valid again | up | OPERATIONAL |
| any (self-test) | n/a | n/a | FAULT if self-test fails |

## Rules that follow from this
- A stale fix is treated as no fix, so old coordinates never keep the device in OPERATIONAL.
- The modem starts bring-up in parallel with GPS acquisition, but the state machine still waits for GPS first, then the network. Whether this order is right is PENDING CONFIRMATION.
- A modem failure is terminal in the mock until a reset. Automatic modem restart from DEGRADED is not implemented.

## Test evidence (simulated)
`integration_week3_test.ino`: normal start-up, GPS loss and recovery, stale fix, modem registration denied, GPS acquire timeout, self-test failure. (8 checks; my host run passed all, to be re-run in Wokwi.)

## Pending Confirmation
1. Does a GPS or network timeout lead to DEGRADED or FAULT?
2. Sequential (GPS then network) versus parallel acquisition.
3. Should the device keep operating with no GPS if the network is up (and vice versa), and what does it report?
4. Modem restart and retry policy from DEGRADED.
5. How device status maps to the agreed `vehicle_status` and `network_status` values.
