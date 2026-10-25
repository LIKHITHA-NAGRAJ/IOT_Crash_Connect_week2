# Event Retry, Duplicate Handling and Offline Results (SIMULATED)

> **All results come from software simulation:** a mock transport and mock backend, a RAM model of flash, a simulated clock and a simulated button signal. An ACK here is NOT a real backend reply. Nothing was sent to the real backend, and no physical button, modem or flash was used.
> Source: `Tests/Test_Suites/tests_events.cpp`, sections C and D. The figures below come from the assertions in those tests (host build). Replace or confirm them with your own Wokwi run (`wokwi_serial_output_week3.txt`).

## Retry and acknowledgement
| Case | Setup | Result |
|---|---|---|
| Online, normal | Event queued, transport acknowledges | 1 attempt, ACK, queue empty, backend has 1 unique event |
| Offline then online | 3 events queued while the network is down (about 5 s of ticks) | 0 send attempts while offline, 3 events kept; when the network returns all 3 are sent oldest first and acknowledged |
| Transmission fails twice | First 2 sends fail, third succeeds | Attempts at 0 ms, 2000 ms and 6000 ms (delay 2 s, then 4 s, doubling, capped at 60 s); no attempt before the retry time; same event ID and identical payload every time; ACK on the third attempt |
| Backend rejects the payload | Corrupted payload (unknown event type) | Marked REJECTED, kept for inspection, not retried (no further sends over about 19 s of ticks) |

## Duplicate-event handling
| Case | Setup | Result |
|---|---|---|
| ACK lost | Backend receives the event but the first ACK is lost | Event stays queued; retry sent with the same event ID and identical payload; mock backend records 2 receptions, **1 unique event, 1 duplicate**; then ACK and the queue empties |
| Retries in general | Any retry | Event ID and payload bytes never change between attempts, so a backend that deduplicates on `event_id` stores one event |
| Reboot while offline | 2 events queued, simulated reboot | Both restored unchanged from the simulated flash; boot counter 1 to 2; a new event after the reboot gets a different ID (`...-B0002-...`); the restored events are sent with their original IDs |
| Queue full | 7 events pushed into a queue of 6 while offline | Oldest dropped, drop counted; policy pending |

## SOS and accident integration (device side, simulated)
- Accident detected by the staged detector (simulated frontal impact): event queued with accident type `impact`, confirmation `confirmed`, current location with age; state OPERATIONAL to EVENT_ACTIVE; event sent and (mock) acknowledged.
- Manual SOS long press while OPERATIONAL with a valid recorded fix: SOS event queued with location and age; state to EVENT_ACTIVE; sent and (mock) acknowledged.
- SOS during start-up, no fix, no network: event queued with null location (no fake coordinates); the start-up state is not disturbed; sent when the network becomes available.
- GPS lost after a good fix: SOS carries the last valid location with its age and `gnss_fix_quality` 0.

## SOS to dashboard (Full Stack): NOT DONE
No event has been sent to the real backend, so acceptance and dashboard display are **not verified**. Reason: the backend endpoint and the confirmed event contract are still needed from Full Stack. Mock valid and invalid payloads and a checklist are ready in `Test_Data/Sample_Events/` and `Firmware/SOS_Events/Event_API_Test_Cases.md`.

## Remaining issues
- Event contract unconfirmed: extra fields, `timestamp_utc` format, `gnss_fix_quality` type, device ID format.
- Last-known location with age versus the null-when-no-fix packet rule.
- Retry limits, queue size and drop policy, maximum age of queued events, priority between event types.
- Real flash storage, wear and power-loss safety are not modelled.
- SOS button, pin and circuit not confirmed with PCB.
