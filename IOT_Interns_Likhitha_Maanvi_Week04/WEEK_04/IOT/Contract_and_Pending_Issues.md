# Contract compatibility and pending issues

## Contract used (flat JSON, no nested objects)
Common: `device_id`, `schema_version` "1.0", `message_type`, `sequence_number`, `timestamp_utc`, `time_source`.
Types follow Cathy's clarifications: `timestamp_utc` integer UTC epoch seconds (null when no trusted time), `gnss_fix_quality` integer (0 when no fix), `latitude`/`longitude`/`speed_kmh` null without a fix, acceleration in g, gyro in deg/s. Event fields follow Puneeth's sample plus `event_id`: `event_type` "accident"/"sos", `description`, `snapshot` (null). Samples: `Test_Results/sample_*.json`.

## Mismatches with Puneeth's sample (to raise)
| Item | Puneeth's sample | Firmware v0.4 | Action |
|---|---|---|---|
| timestamp_utc | ISO string "2026-10-21T11:30:00Z" | integer epoch seconds | confirm one format |
| gnss_fix_quality | string "valid" | integer 0, 1, 2 ... | confirm |
| event_id | not in the sample | `<device_id>-B<boot_count>-E<n>` | confirm backend does not generate it |
| sequence_number | integer | per-boot counter, restarts at 1 after a reset | persist? |

## Fields I added (not agreed, pending Full Stack)
`message_type`, `time_source`, `data_source`, `gnss_status`, `course_deg`, `satellites`, `hdop`, `last_fix_age_s`, `fault_flags`, `device_state`; event: `location_age_s`, `location_source` (CURRENT / LAST_KNOWN / NONE), `accident_type`, `confirmation_state` (CONFIRMED / USER_TRIGGERED), `peak_accel_g`; status: `firmware_version`, `uptime_s`, `reset_reason`, `boot_count`, `provisioned`, `network_registered`, `signal_csq`, `signal_dbm` (derived as 2 x CSQ - 113), `power_source` (null), `battery_mv` (null).

## Draft answers to Puneeth's six questions (21 Oct)
1. event_id: the device creates it, format above, kept identical on every retry.
2. event_type values: `accident`, `sos`.
3. Timestamp: field `timestamp_utc`; the firmware sends integer UTC epoch seconds, not ISO (please confirm which you want).
4. Location: `latitude` and `longitude` are always present; both null and `location_source` "NONE" when no position was ever obtained. Otherwise the last valid position with `location_age_s`.
5. State: the device sends only `confirmation_state` (CONFIRMED or USER_TRIGGERED). NEW / ACKNOWLEDGED / CLOSED / MARKED_FALSE_ALARM stay on the backend.
6. Examples: `Test_Results/sample_accident_event.json`, `sample_sos_event.json`, `sample_event_last_known_location.json`.

## Open issues and hardware-dependent items
1. **Not run on ESP32-S3 or in Wokwi.** Code behind `ARDUINO_ARCH_ESP32` (Preferences, task watchdog, reset reason) is untested.
2. **Hardware access layers not written:** GNSS UART read, LSM6DSOX driver, modem UART port and PWRKEY, SOS GPIO, the `setup()/loop()` entry point. Pins are not confirmed. `app_setup` and `app_loop` are what the entry point should call.
3. Queue is RAM only (lost on reset); flash-backed queue not implemented.
4. Accident thresholds are placeholders; scenarios are synthetic; `device_knock_no_gps` is a known false positive.
5. An in-flight telemetry send can delay a new event by up to the modem HTTP timeout (seen in the demo: SOS raised at 108 s was first sent at 113.5 s). Improvement: abort the in-flight telemetry send when an event arrives.
6. AT syntax unverified, JWT and HTTPS missing (see `Modem_Data_Approach.md`).
7. Battery and power status unavailable (null): no ADC pin or fuel gauge defined. Battery is the 1200 mAh Li cell (changed 20 Oct); Week 2 documents still mention 3400 mAh.
8. Latency: only mock figures (Maanvi's 50 ms injected delay; demo SOS raised 87.0 s, mock ACK 87.5 s). Not measured with Puneeth.
9. Overflow policies need approval (see `Configuration_Notes.md`).
10. Timestamps in the demo come from generated NMEA dated 31 Oct 2026, not a real receiver.
