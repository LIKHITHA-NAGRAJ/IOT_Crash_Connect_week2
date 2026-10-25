# Test Cases (Week 3, IoT)

> **All inputs are RECORDED, SIMULATED or MOCKED.** A pass shows the software logic works. It does NOT validate real GNSS, IMU, modem, network, flash, a physical SOS button or real accident detection.
> The "PC build" column is the result of a PC build of the same code (reference). The "Wokwi result" column is for YOU to fill in from your own Wokwi run, which is the evidence for the review.
> Where to run: copy all firmware and `Tests/Test_Suites` files into one Wokwi ESP32-S3 project (see README). `sketch.ino` runs every suite.

## Summary
| Suite | Source file | Input type | Checks |
|---|---|---|---|
| GNSS | `tests_gnss.cpp` | Recorded NMEA (parser tests only) | 31 |
| MODEM | `tests_modem.cpp` | MOCK AT-command responses | 14 |
| INTEG | `tests_integration.cpp` | Recorded NMEA + MOCK modem | 8 |
| ACC | `tests_accident.cpp` | SIMULATED samples | 42 |
| EVT | `tests_events.cpp` | SIMULATED button, mock transport/backend, simulated flash | 56 |
| LEGACY | `tests_telemetry_legacy.cpp` | Week 2 nested draft packet | manual review of output (6 scenarios) |
| **Total self-checking** | | | **151** |

## GNSS: `tests_gnss.cpp`
Input type: Recorded NMEA (parser tests only)

| ID | Group | Test case | PC build | Wokwi result |
|---|---|---|---|---|
| GNSS-01 | Test 1: valid recorded pair | fix is valid | PASS | [ ] |
| GNSS-02 | Test 1: valid recorded pair | fix quality comes from GGA (1) | PASS | [ ] |
| GNSS-03 | Test 1: valid recorded pair | satellites 8 and HDOP 0.9 parsed | PASS | [ ] |
| GNSS-04 | Test 1: valid recorded pair | speed 22.4 knots parsed | PASS | [ ] |
| GNSS-05 | Test 1: valid recorded pair | UTC epoch seconds computed (764426119) | PASS | [ ] |
| GNSS-06 | Test 1: valid recorded pair | course 84.4 degrees parsed | PASS | [ ] |
| GNSS-07 | Test 1: valid recorded pair | status text is VALID | PASS | [ ] |
| GNSS-08 | Test 1: valid recorded pair | time flagged NOT live (recorded data) | PASS | [ ] |
| GNSS-09 | Test 2: recorded no-fix pair | fix not valid | PASS | [ ] |
| GNSS-10 | Test 2: recorded no-fix pair | fix quality 0 | PASS | [ ] |
| GNSS-11 | Test 2: recorded no-fix pair | speed and UTC not valid | PASS | [ ] |
| GNSS-12 | Test 2: recorded no-fix pair | age = never had a fix | PASS | [ ] |
| GNSS-13 | Test 2: recorded no-fix pair | status text is NO_FIX | PASS | [ ] |
| GNSS-14 | Test 2: recorded no-fix pair | course not valid | PASS | [ ] |
| GNSS-15 | Test 2: recorded no-fix pair | no last-valid fix exists | PASS | [ ] |
| GNSS-16 | Test 3: too few satellites (3 < placeholder minimum) | rejected, quality 0 | PASS | [ ] |
| GNSS-17 | Test 4: HDOP too high (9.9 > placeholder maximum) | rejected, quality 0 | PASS | [ ] |
| GNSS-18 | Test 5: corrupted line (bad checksum) | no fix: corrupted RMC was rejected | PASS | [ ] |
| GNSS-19 | Test 6: fix becomes old -> must not be shown as current | valid right after the lines | PASS | [ ] |
| GNSS-20 | Test 6: fix becomes old -> must not be shown as current | after the stale limit: not valid, quality 0 | PASS | [ ] |
| GNSS-21 | Test 6: fix becomes old -> must not be shown as current | age of last valid fix is reported | PASS | [ ] |
| GNSS-22 | Test 6: fix becomes old -> must not be shown as current | status text is NO_FIX when stale | PASS | [ ] |
| GNSS-23 | Test 6: fix becomes old -> must not be shown as current | last-valid fix kept as HISTORY with its age | PASS | [ ] |
| GNSS-24 | Test 6: fix becomes old -> must not be shown as current | history is available, but fix.valid stays false (not current) | PASS | [ ] |
| GNSS-25 | Test 7: valid fix lost -> no-fix sentences arrive | reported as no fix immediately | PASS | [ ] |
| GNSS-26 | Test 8: GPS loss and recovery | step 1: valid fix | PASS | [ ] |
| GNSS-27 | Test 8: GPS loss and recovery | step 2: fix lost -> NO_FIX, quality 0 | PASS | [ ] |
| GNSS-28 | Test 8: GPS loss and recovery | step 2: old fix kept only as history | PASS | [ ] |
| GNSS-29 | Test 8: GPS loss and recovery | step 3: fix recovered | PASS | [ ] |
| GNSS-30 | Test 8: GPS loss and recovery | step 3: age reset after the new valid fix | PASS | [ ] |
| GNSS-31 | Test 9: malformed / invalid input must not crash or create a fix | garbage, truncated and too-short lines ignored; NO_FIX | PASS | [ ] |

## MODEM: `tests_modem.cpp`
Input type: MOCK AT-command responses

| ID | Group | Test case | PC build | Wokwi result |
|---|---|---|---|---|
| MODEM-01 | Test 1: everything works (mock) | reaches READY_SIMULATED | PASS | [ ] |
| MODEM-02 | Test 1: everything works (mock) | SIM ready, registered, data session flags (all mock) | PASS | [ ] |
| MODEM-03 | Test 1: everything works (mock) | signal quality read (mock CSQ 18) | PASS | [ ] |
| MODEM-04 | Test 1: everything works (mock) | status is flagged simulated | PASS | [ ] |
| MODEM-05 | Test 2: modem never answers AT | FAILED_NO_RESPONSE after the retry limit | PASS | [ ] |
| MODEM-06 | Test 2: modem never answers AT | exactly maxRetries attempts were made | PASS | [ ] |
| MODEM-07 | Test 3: SIM not inserted | FAILED_NO_SIM | PASS | [ ] |
| MODEM-08 | Test 4: registration denied | FAILED_REG_DENIED | PASS | [ ] |
| MODEM-09 | Test 5: network never registers (timeout) | FAILED_REG_TIMEOUT after the registration timeout | PASS | [ ] |
| MODEM-10 | Test 6: weak signal (policy is a placeholder) | weak signal flagged (CSQ 3) | PASS | [ ] |
| MODEM-11 | Test 6: weak signal (policy is a placeholder) | bring-up continues (policy PENDING CONFIRMATION) | PASS | [ ] |
| MODEM-12 | Test 7: data session cannot be opened | FAILED_DATA_SESSION | PASS | [ ] |
| MODEM-13 | Test 7: data session cannot be opened | no data session flag | PASS | [ ] |
| MODEM-14 | Test 8: APN still PENDING (as in config) | FAILED_APN_PENDING, no invented APN used | PASS | [ ] |

## INTEG: `tests_integration.cpp`
Input type: Recorded NMEA + MOCK modem

| ID | Group | Test case | PC build | Wokwi result |
|---|---|---|---|---|
| INTEG-01 | Test 1: normal start-up | reaches OPERATIONAL (mock network) | PASS | [ ] |
| INTEG-02 | Test 2: GPS loss then recovery (while OPERATIONAL) | GPS lost -> DEGRADED | PASS | [ ] |
| INTEG-03 | Test 2: GPS loss then recovery (while OPERATIONAL) | GPS recovered -> OPERATIONAL | PASS | [ ] |
| INTEG-04 | Test 3: GPS data stops (last fix goes stale) | stale fix is not current -> DEGRADED | PASS | [ ] |
| INTEG-05 | Test 4: modem fails (registration denied, mock), GPS fine | modem ends in FAILED_REG_DENIED (mock) | PASS | [ ] |
| INTEG-06 | Test 4: modem fails (registration denied, mock), GPS fine | network failure -> DEGRADED | PASS | [ ] |
| INTEG-07 | Test 5: GPS never delivers a fix (acquire timeout) | GPS acquire timeout -> DEGRADED (PENDING CONFIRMATION) | PASS | [ ] |
| INTEG-08 | Test 6: self-test fails | SELF_TEST failure -> FAULT | PASS | [ ] |

## ACC: `tests_accident.cpp`
Input type: SIMULATED samples

| ID | Group | Test case | PC build | Wokwi result |
|---|---|---|---|---|
| ACC-01 | Run A: false positives = 2, missed = 0 | no ideal accident was missed in run A (simulated) | PASS | [ ] |
| ACC-02 | Run A: false positives = 2, missed = 0 | exactly the 2 documented false-positive risks are reported (bump_then_hard_stop, bump_gps_missing_smooth) | PASS | [ ] |
| ACC-03 | Run A: false positives = 2, missed = 0 | raising the impact threshold removes the frontal-impact candidate | PASS | [ ] |
| ACC-04 | Run A: false positives = 2, missed = 0 | requireOrientation rejects frontal impact but keeps rollover-like | PASS | [ ] |
| ACC-05 | Pipeline to state machine (simulated): | device is OPERATIONAL before the replay | PASS | [ ] |
| ACC-06 | Pipeline to state machine (simulated): | confirmed event is latched | PASS | [ ] |
| ACC-07 | Pipeline to state machine (simulated): | OPERATIONAL -> EVENT_ACTIVE | PASS | [ ] |
| ACC-08 | Pipeline to state machine (simulated): | event hold elapsed -> back to OPERATIONAL | PASS | [ ] |
| ACC-09 | Pipeline to state machine (simulated): | GPS lost -> DEGRADED | PASS | [ ] |
| ACC-10 | Pipeline to state machine (simulated): | event while DEGRADED -> EVENT_ACTIVE (not dropped) | PASS | [ ] |
| ACC-11 | Pipeline to state machine (simulated): | pothole scenario: no event latched | PASS | [ ] |
| ACC-12 | Pipeline to state machine (simulated): | pothole scenario: one candidate, rejected | PASS | [ ] |
| ACC-13 | Pipeline to state machine (simulated): | recorded GNSS speed 22.4 kn -> 41.5 km/h for the detector | PASS | [ ] |
| ACC-14 | Pipeline to state machine (simulated): | no GNSS fix -> speed unavailable for the detector | PASS | [ ] |

### ACC scenario regression checks (silent unless they fail; details in `Scenario_Results.csv`)
| ID | Scenario | Configuration | Expected | PC build | Wokwi result |
|---|---|---|---|---|---|
| ACC-S01 | normal_cruise | default | 0 candidates, 0 confirmed | PASS | [ ] |
| ACC-S02 | gentle_accel_stop | default | 0 candidates, 0 confirmed | PASS | [ ] |
| ACC-S03 | hard_braking | default | 0 candidates, 0 confirmed | PASS | [ ] |
| ACC-S04 | cornering_handling | default | 0 candidates, 0 confirmed | PASS | [ ] |
| ACC-S05 | pothole_single | default | 0 candidates, 0 confirmed | PASS | [ ] |
| ACC-S06 | pothole_double_hit | default | 1 candidates, 0 confirmed | PASS | [ ] |
| ACC-S07 | bump_then_hard_stop | default | 1 candidates, 1 confirmed | PASS | [ ] |
| ACC-S08 | parked_door_slam | default | 1 candidates, 0 confirmed | PASS | [ ] |
| ACC-S09 | minor_contact_drive | default | 1 candidates, 0 confirmed | PASS | [ ] |
| ACC-S10 | frontal_impact_stop | default | 1 candidates, 1 confirmed | PASS | [ ] |
| ACC-S11 | rollover_like | default | 1 candidates, 1 confirmed | PASS | [ ] |
| ACC-S12 | impact_gps_missing | default | 1 candidates, 1 confirmed | PASS | [ ] |
| ACC-S13 | bump_gps_missing_rough | default | 1 candidates, 0 confirmed | PASS | [ ] |
| ACC-S14 | bump_gps_missing_smooth | default | 1 candidates, 1 confirmed | PASS | [ ] |
| ACC-S15 | normal_cruise | jerk trigger on | 0 candidates | PASS | [ ] |
| ACC-S16 | gentle_accel_stop | jerk trigger on | 0 candidates | PASS | [ ] |
| ACC-S17 | hard_braking | jerk trigger on | 0 candidates | PASS | [ ] |
| ACC-S18 | cornering_handling | jerk trigger on | 0 candidates | PASS | [ ] |
| ACC-S19 | pothole_single | jerk trigger on | 1 candidates | PASS | [ ] |
| ACC-S20 | pothole_double_hit | jerk trigger on | 1 candidates | PASS | [ ] |
| ACC-S21 | bump_then_hard_stop | jerk trigger on | 1 candidates | PASS | [ ] |
| ACC-S22 | parked_door_slam | jerk trigger on | 1 candidates | PASS | [ ] |
| ACC-S23 | minor_contact_drive | jerk trigger on | 1 candidates | PASS | [ ] |
| ACC-S24 | frontal_impact_stop | jerk trigger on | 1 candidates | PASS | [ ] |
| ACC-S25 | rollover_like | jerk trigger on | 1 candidates | PASS | [ ] |
| ACC-S26 | impact_gps_missing | jerk trigger on | 1 candidates | PASS | [ ] |
| ACC-S27 | bump_gps_missing_rough | jerk trigger on | 1 candidates | PASS | [ ] |
| ACC-S28 | bump_gps_missing_smooth | jerk trigger on | 1 candidates | PASS | [ ] |

## EVT: `tests_events.cpp`
Input type: SIMULATED button, mock transport/backend, simulated flash

| ID | Group | Test case | PC build | Wokwi result |
|---|---|---|---|---|
| EVT-01 | A. Manual SOS long press (simulated signal) | A1 contact bounce is ignored (no press registered) | PASS | [ ] |
| EVT-02 | A. Manual SOS long press (simulated signal) | A2 short press: feedback RELEASED_EARLY, no SOS | PASS | [ ] |
| EVT-03 | A. Manual SOS long press (simulated signal) | A3 long press triggers exactly once (not again while still held) | PASS | [ ] |
| EVT-04 | A. Manual SOS long press (simulated signal) | A3 trigger happens at the configured hold time | PASS | [ ] |
| EVT-05 | A. Manual SOS long press (simulated signal) | A3 progress feedback was reported while holding | PASS | [ ] |
| EVT-06 | A. Manual SOS long press (simulated signal) | A4 press during cooldown is ignored | PASS | [ ] |
| EVT-07 | A. Manual SOS long press (simulated signal) | A5 long press after the cooldown triggers again | PASS | [ ] |
| EVT-08 | B. Event payloads (recorded sample coordinates, simulated time) | B1 SOS payload built | PASS | [ ] |
| EVT-09 | B. Event payloads (recorded sample coordinates, simulated time) | B1 has event ID, type, integer timestamp, location age, fix quality, snapshot null, simulated flag | PASS | [ ] |
| EVT-10 | B. Event payloads (recorded sample coordinates, simulated time) | B1 accepted by the mock backend rules | PASS | [ ] |
| EVT-11 | B. Event payloads (recorded sample coordinates, simulated time) | B2 no location and no time: everything null, no invented coordinates | PASS | [ ] |
| EVT-12 | B. Event payloads (recorded sample coordinates, simulated time) | B2 null location accepted by the mock backend rules | PASS | [ ] |
| EVT-13 | B. Event payloads (recorded sample coordinates, simulated time) | B3 last-known location is sent with its age, fix quality 0 (no current fix) | PASS | [ ] |
| EVT-14 | B. Event payloads (recorded sample coordinates, simulated time) | B4 accident event has type, accident type and confirmation state | PASS | [ ] |
| EVT-15 | B. Event payloads (recorded sample coordinates, simulated time) | B4 rollover and low-confidence variants | PASS | [ ] |
| EVT-16 | B. Event payloads (recorded sample coordinates, simulated time) | B5 ISO 8601 option produces the sample's text format | PASS | [ ] |
| EVT-17 | B. Event payloads (recorded sample coordinates, simulated time) | B6 buffer too small: returns 0 and an empty string | PASS | [ ] |
| EVT-18 | B. Event payloads (recorded sample coordinates, simulated time) | B7 current fix wins | PASS | [ ] |
| EVT-19 | B. Event payloads (recorded sample coordinates, simulated time) | B7 last-known location used with its age, quality 0 | PASS | [ ] |
| EVT-20 | B. Event payloads (recorded sample coordinates, simulated time) | B7 last-known location older than the limit is reported as unavailable | PASS | [ ] |
| EVT-21 | B. Event payloads (recorded sample coordinates, simulated time) | B7 never had a fix: unavailable | PASS | [ ] |
| EVT-22 | B. Event payloads (recorded sample coordinates, simulated time) | B8 missing event_id rejected | PASS | [ ] |
| EVT-23 | B. Event payloads (recorded sample coordinates, simulated time) | B8 unknown event_type rejected | PASS | [ ] |
| EVT-24 | B. Event payloads (recorded sample coordinates, simulated time) | B8 latitude out of range rejected | PASS | [ ] |
| EVT-25 | B. Event payloads (recorded sample coordinates, simulated time) | B8 ISO text rejected when an integer timestamp is expected | PASS | [ ] |
| EVT-26 | B. Event payloads (recorded sample coordinates, simulated time) | B8 latitude present but longitude null rejected | PASS | [ ] |
| EVT-27 | C. Queue, retry and acknowledgement (MOCK transport and backend, simulated flash) | C1 online: sent once, acknowledged, queue empty | PASS | [ ] |
| EVT-28 | C. Queue, retry and acknowledgement (MOCK transport and backend, simulated flash) | C2 offline: nothing is sent, 3 events wait in the queue | PASS | [ ] |
| EVT-29 | C. Queue, retry and acknowledgement (MOCK transport and backend, simulated flash) | C2 connectivity back: all 3 events sent and acknowledged | PASS | [ ] |
| EVT-30 | C. Queue, retry and acknowledgement (MOCK transport and backend, simulated flash) | C2 events are sent oldest first | PASS | [ ] |
| EVT-31 | C. Queue, retry and acknowledgement (MOCK transport and backend, simulated flash) | C3 first failure: retry scheduled after the base delay | PASS | [ ] |
| EVT-32 | C. Queue, retry and acknowledgement (MOCK transport and backend, simulated flash) | C3 no attempt before the retry time | PASS | [ ] |
| EVT-33 | C. Queue, retry and acknowledgement (MOCK transport and backend, simulated flash) | C3 second failure: delay doubled | PASS | [ ] |
| EVT-34 | C. Queue, retry and acknowledgement (MOCK transport and backend, simulated flash) | C3 payload and event ID unchanged across retries | PASS | [ ] |
| EVT-35 | C. Queue, retry and acknowledgement (MOCK transport and backend, simulated flash) | C3 third attempt acknowledged; backend got the identical payload | PASS | [ ] |
| EVT-36 | C. Queue, retry and acknowledgement (MOCK transport and backend, simulated flash) | C4 ACK lost: the event stays queued | PASS | [ ] |
| EVT-37 | C. Queue, retry and acknowledgement (MOCK transport and backend, simulated flash) | C4 retry with the same ID: backend sees a duplicate (1 unique event), then ACK | PASS | [ ] |
| EVT-38 | C. Queue, retry and acknowledgement (MOCK transport and backend, simulated flash) | C5 rejected payload: marked REJECTED, kept for inspection, not retried | PASS | [ ] |
| EVT-39 | C. Queue, retry and acknowledgement (MOCK transport and backend, simulated flash) | C6 first boot is boot 1 | PASS | [ ] |
| EVT-40 | C. Queue, retry and acknowledgement (MOCK transport and backend, simulated flash) | C6 after a reboot the events are restored unchanged and the boot counter increased | PASS | [ ] |
| EVT-41 | C. Queue, retry and acknowledgement (MOCK transport and backend, simulated flash) | C6 a new event after the reboot gets a new, unique ID | PASS | [ ] |
| EVT-42 | C. Queue, retry and acknowledgement (MOCK transport and backend, simulated flash) | C6 restored events are sent with their original IDs | PASS | [ ] |
| EVT-43 | C. Queue, retry and acknowledgement (MOCK transport and backend, simulated flash) | C7 queue full: oldest event dropped (policy PENDING CONFIRMATION), count reported | PASS | [ ] |
| EVT-44 | D. SOS and accident events with GPS validity and the state machine (simulated) | D1 confirmed accident created and queued one event | PASS | [ ] |
| EVT-45 | D. SOS and accident events with GPS validity and the state machine (simulated) | D1 accident type and confirmation state present | PASS | [ ] |
| EVT-46 | D. SOS and accident events with GPS validity and the state machine (simulated) | D1 location from the current recorded fix, quality 1, simulated integer timestamp | PASS | [ ] |
| EVT-47 | D. SOS and accident events with GPS validity and the state machine (simulated) | D1 state machine: OPERATIONAL to EVENT_ACTIVE | PASS | [ ] |
| EVT-48 | D. SOS and accident events with GPS validity and the state machine (simulated) | D1 event sent and (mock) acknowledged | PASS | [ ] |
| EVT-49 | D. SOS and accident events with GPS validity and the state machine (simulated) | D2 long press created an SOS event with the current location and its age | PASS | [ ] |
| EVT-50 | D. SOS and accident events with GPS validity and the state machine (simulated) | D2 state machine: SOS moves OPERATIONAL to EVENT_ACTIVE | PASS | [ ] |
| EVT-51 | D. SOS and accident events with GPS validity and the state machine (simulated) | D2 SOS sent and (mock) acknowledged | PASS | [ ] |
| EVT-52 | D. SOS and accident events with GPS validity and the state machine (simulated) | D3 no fix and never had one: location unavailable, no fake coordinates | PASS | [ ] |
| EVT-53 | D. SOS and accident events with GPS validity and the state machine (simulated) | D3 start-up state is not disturbed | PASS | [ ] |
| EVT-54 | D. SOS and accident events with GPS validity and the state machine (simulated) | D3 no network: the SOS stays queued | PASS | [ ] |
| EVT-55 | D. SOS and accident events with GPS validity and the state machine (simulated) | D3 network available: the queued SOS is sent | PASS | [ ] |
| EVT-56 | D. SOS and accident events with GPS validity and the state machine (simulated) | D4 GPS lost: last valid location sent with its age, fix quality 0 | PASS | [ ] |

## LEGACY: Week 2 nested telemetry draft (manual review)
The packet is NOT aligned to the agreed flat schema. Timestamp is null for recorded data.

- TEST 1: normal start-up (GPS from recorded NMEA, network = simulated)
- TEST 2: no GPS fix -> location and timestamp must be null
- TEST 3: IMU unavailable -> IMU values must be null
- TEST 4: movement burst -> EVENT_ACTIVE (SIMULATED test values)
- TEST 5: GPS fix lost -> DEGRADED, then recovery
- TEST 6: self-test fails -> FAULT; illegal transition rejected

## Not covered
Real GNSS reception, real IMU data, real modem and network, real flash, a physical button, API acceptance and the dashboard (not done), the flat telemetry schema (not available), and real accident-detection performance.
