# Event API Integration Checklist (with Full Stack)

> All payloads are MOCK/SIMULATED (see `Sample_Events/`). The IoT side has NOT sent anything to the backend. These steps are for testing the backend with the same payloads a device would send.

## What Full Stack is asked to confirm
1. Does the real contract include `event_id`, `location_age_s`, `accident_type`, `confirmation_state`, `simulated`? If not, which should the backend ignore or reject?
2. `timestamp_utc`: integer epoch seconds or ISO 8601 text? Is null allowed?
3. `gnss_fix_quality`: integer or text? Which values?
4. Are `latitude` and `longitude` allowed to be null? What does the backend do with a last-known location (age > 0, fix quality 0)?
5. Event lifecycle (`NEW`, `ACKNOWLEDGED`, `CLOSED`, `MARKED_FALSE_ALARM`) is assumed to be owned by the backend, and an accepted event starts as `NEW`. Confirm.
6. Endpoint, authentication, success and error response format, and what the device should treat as an acknowledgement.

## Steps (Postman or equivalent)
Endpoint and headers: `[fill in from Full Stack]`.
| # | Action | Expected |
|---|---|---|
| 1 | POST `valid_sos_current_location.json` | Accepted, ACK returned, event shows in the dashboard as NEW |
| 2 | POST the identical body again | Not stored twice; same `event_id` recognised; ACK returned |
| 3 | POST `valid_sos_last_known_location.json` | Accepted; dashboard shows the position with its age, not as a current location |
| 4 | POST `valid_sos_no_location_no_time.json` | Accepted if null is allowed; dashboard shows "location unavailable" |
| 5 | POST `valid_accident_impact_confirmed.json` and `valid_accident_rollover_low_confidence.json` | Accepted; accident type and confirmation state visible |
| 6 | POST each `invalid_*.json` | Rejected with a clear error and not shown in the dashboard |
| 7 | Stop the backend, POST from the simulator again | (device side) the event stays queued and is retried with the same ID |

## Result table (Full Stack fills in)
| File | Expected | Actual response | In dashboard? |
|---|---|---|---|
| valid_sos_current_location | accept | [ ] | [ ] |
| valid_sos_iso_timestamp | depends on contract | [ ] | [ ] |
| valid_sos_last_known_location | accept | [ ] | [ ] |
| valid_sos_no_location_no_time | accept if null allowed | [ ] | [ ] |
| valid_accident_impact_confirmed | accept | [ ] | [ ] |
| valid_accident_rollover_low_confidence | accept | [ ] | [ ] |
| invalid_missing_event_id | reject | [ ] | [ ] |
| invalid_unknown_event_type | reject | [ ] | [ ] |
| invalid_latitude_out_of_range | reject | [ ] | [ ] |
| invalid_timestamp_text_when_integer_expected | reject | [ ] | [ ] |
| invalid_latitude_without_longitude | reject | [ ] | [ ] |
| invalid_fix_quality_text_when_integer_expected | reject | [ ] | [ ] |

## Status
Not yet run. This checklist needs the backend endpoint and the confirmed contract.
