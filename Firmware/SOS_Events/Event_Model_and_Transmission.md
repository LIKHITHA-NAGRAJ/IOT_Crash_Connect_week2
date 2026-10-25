# SOS, Accident Events, Retry and Offline Queue (design)

> **Everything is SIMULATED.** The SOS button is a boolean signal, the "flash" is a RAM copy, the modem and backend are mocks, and time is the simulator's. An ACK in these tests is NOT a real backend reply, and nothing was transmitted. Passing tests prove the software logic only, not any physical button, modem, flash or backend.

## Code map and ownership
| Part | Files | Suggested owner |
|---|---|---|
| Long-press, debounce, feedback | `sos_button.*` | Manvi |
| Event payload construction | `event_builder.*` | Manvi |
| Retry, acknowledgement, queue (simulated flash) | `event_queue.*`, `event_transport.*` | Manvi |
| Event creation with GPS validity and time, state-machine integration | `event_manager.*`, `device_integration.*`, `state_machine.*` | Likhitha |
Agree interface changes before editing each other's files.

## Event structure
Base fields follow the sample event model from Full Stack. Fields marked PROPOSED are not in that sample and need confirmation.

| Field | Type | Notes |
|---|---|---|
| event_id | string | **PROPOSED.** Generated on the device: `<device_id>-B<boot>-<sequence>`. Unique, persisted counters, never reused, identical on every retry. The backend may prefer a UUID (open question) |
| device_id | string | From config (`CC-IOT-01`). The sample uses `DEVICE-001`: format to be agreed |
| schema_version | string | `"1.0"` as in the sample. Added fields may need a new version |
| sequence_number | integer | Per-device event counter, persisted |
| event_type | string | `"accident"` or `"sos"` |
| timestamp_utc | integer or null | Integer UTC epoch seconds (Technical Lead). The sample shows ISO 8601 text: **mismatch, pending**. A config switch (`isoTimestamp`) produces either |
| description | string | Fixed text per type, as in the sample |
| latitude, longitude | number or null | See the location rule below |
| location_age_s | integer or null | **PROPOSED.** Age of the location in seconds |
| gnss_fix_quality | integer | Quality of the CURRENT fix, 0 = no fix. The sample shows text (`"valid"`): **mismatch, pending** |
| accident_type | string | **PROPOSED.** Accident only: `impact` or `rollover_suspected` |
| confirmation_state | string | **PROPOSED.** Accident only: `confirmed` or `confirmed_low_confidence` |
| snapshot | null | Always null for now. Content (for example a pre-event IMU buffer) not defined |
| simulated | boolean | **PROPOSED.** Labels simulation-mode events; remove for real hardware |

## Time rule
A recorded GNSS time is not live, so it is never used. A real build would use live GNSS time (plus elapsed time); without any valid time, `timestamp_utc` is null. Tests use a labelled SIMULATED clock. The time source when there is no GNSS fix is still an open question.

## Location rule (IMPORTANT, needs confirmation)
1. A **current valid fix**: its position, `location_age_s` is its age, `gnss_fix_quality` is the fix quality.
2. **No current fix, but a last valid fix no older than `maxLastKnownAgeS`** (600 s placeholder): the last known position with its age, and `gnss_fix_quality` = 0.
3. Otherwise latitude, longitude and age are null.
No coordinates are ever invented. **Conflict to resolve:** the Week 3 packet rule says latitude and longitude are null when there is no valid fix, while the Thursday task asks events to carry the last valid location with its age. Events follow the Thursday task; telemetry still follows the packet rule. `location_age_s` > 0 with `gnss_fix_quality` 0 means "last known, not current". The Technical Lead and Full Stack must confirm this.

## Manual SOS
Debounce (30 ms), a long press of 2.5 s (configurable, example range 2 to 3 s), progress feedback while holding, a cooldown (5 s) after a trigger. Feedback is reported as `PRESS_STARTED`, `HOLDING` with a percentage, `TRIGGERED`, `RELEASED_EARLY`, `IGNORED_COOLDOWN`; they stand in for LED or buzzer output. A completed press creates an SOS event at once. The button, its pin (GPIO13 proposed) and its circuit are not confirmed with PCB.

## Accident event
Created when the staged detector confirms an event (`accident_detector.*`, simulated data only). `accident_type` is `rollover_suspected` when the orientation changed or a gyro spike occurred, otherwise `impact`. `confirmation_state` is `confirmed_low_confidence` when no GPS speed was available.

## Queue, retry and acknowledgement
- Events are created and queued in **any** device state, including start-up, so none is lost. The state machine enters EVENT_ACTIVE only from OPERATIONAL or DEGRADED.
- One send attempt per tick, oldest event first, only when the network is up.
- Failure or missing ACK: the event stays queued and is retried with exponential backoff (2 s doubling, capped at 60 s; placeholders). There is no attempt limit for events.
- The same event ID and the same payload bytes are sent on every retry. If the ACK was lost, the retry reaches the backend again; a backend that deduplicates by `event_id` stores one event (tested against the mock backend).
- A rejected payload (invalid) is marked REJECTED, kept for inspection and not retried forever.
- Queue capacity is 6 (placeholder). When full, the oldest pending event is dropped and counted (policy pending).

## Simulated flash model
The queue and its counters (boot counter, next sequence) are copied to a RAM "flash image" after every change, and restored on a simulated reboot. IDs are never reused after a reboot. **Not modelled or validated:** real flash wear and erase blocks, power loss during a write, file-system behaviour, write time. Flash write counts are printed as a rough wear indicator.

## Mock backend (in `event_transport.cpp`)
Checks required fields, event type, integer-or-null timestamp, latitude and longitude range and pairing, and integer fix quality; deduplicates by `event_id`. **These rules are our assumptions**, not Full Stack's real validation.

## Test evidence (56 checks, all simulated)
A (SOS button, 7), B (payloads, location rule, invalid payloads, 19), C (queue, retry, offline, ACK lost, reject, reboot, queue full, 17), D (integration with GPS validity and the state machine, 13). Re-run in Wokwi and use your own output.

## Known limitations
- No physical button, modem, flash or backend was involved.
- No real transmission, so the Full Stack acceptance and dashboard checks have NOT been done.
- Extra fields and the location and timestamp rules are unconfirmed against the real contract.
- No power-loss safety, queue-size policy, time limit for old events, or prioritisation between event types.
- SOS during start-up is queued but the device state does not change.
- EVENT_ACTIVE ends after a hold time, not on acknowledgement.

## Pending Confirmation
See `Firmware/Design_Notes/Pending_Issues.md` (Thursday section).
