# Pending Issues (Week 3, Monday)

## For the PCB team (do not guess)
1. GNSS (u-blox MAX-M10S): which UART pins to the ESP32-S3, baud rate, and whether power, reset or time-pulse pins are used.
2. A7670C: which UART pins and baud rate, PWRKEY and RESET control pins, and any status or ring-indicator pins.
3. Logic voltage levels between the ESP32-S3 and the A7670C (level shifting needed?). To confirm in the datasheets.
4. Modem power: supply path and peak current handling during transmit bursts, and whether the 3.3 V/battery arrangement supports it.
5. SIM: socket type, SIM detect pin if any.
6. Antennas: LTE main (and diversity if used) and GNSS antenna type, connector, placement, and isolation between LTE and GNSS.
7. IMU bus: I2C or SPI, address or chip-select, interrupt pin.

## For the Technical Lead
1. Modem data approach: confirm Option A (modem TCP/MQTT) or choose PPP.
2. MQTT versus REST/TCP, broker or API address, authentication, TLS.
3. APN, SIM and data plan details.
4. Time source when there is no GNSS fix (RTC, network time, or null).
5. Policy for weak signal, and for GPS or network timeouts (DEGRADED versus FAULT).
6. Minimum satellites and maximum HDOP for a valid fix, stale-fix limit, and the accident threshold (placeholders now).

## For Full Stack
See `Firmware/Telemetry/Flat_Packet_Compatibility_Notes.md`.

## Known limitations of today's work
- GNSS uses recorded NMEA for parser tests only; no live GPS, no real receiver.
- The modem state machine uses mock responses; no real registration, data session or transmission.
- Telemetry builder is still the nested Week 2 version (not yet flat).
- Wokwi runs of these tests are still to be done on the intern's side.

## Added Tuesday (state machine and integration)
- Does a GPS or network timeout lead to DEGRADED or FAULT? (Implemented as DEGRADED for testing only.)
- GPS first then network, or both in parallel?
- Modem restart and retry policy from DEGRADED (not implemented; a modem failure is terminal in the mock).
- How device status maps to the agreed `vehicle_status`, `accident_status` and `network_status` values.
- API validation rules for the 14 packet cases in `API_Packet_Test_Cases.md` (waiting on Full Stack).
- Exact agreed flat telemetry schema: still needed before the packet builder can be rewritten.

## Added Wednesday (accident detection)
- Real thresholds and windows, and the logged data needed to set them (all current values are placeholders).
- Should orientation change or a gyro spike be required to confirm? (Default: not required.)
- Mitigation for the documented false positives: a bump followed by a hard stop, and a smooth-road bump with no GPS speed.
- Policy when GPS speed is unavailable (confirm on IMU alone with low confidence, or wait for more evidence).
- Whether events during start-up states should be handled, and whether events take priority in DEGRADED (implemented as yes, for testing).
- Event message, event ID and the SOS flow after confirmation (not built).
- LSM6DSOX range, output data rate and filter options from the datasheet (Manvi), and the real sample rate the detector will run at.
- Mounting orientation and axis mapping (needed before gravity-vector results mean anything on real hardware).

## Added Thursday (SOS, events, transmission)
- Event contract: are `event_id`, `location_age_s`, `accident_type`, `confirmation_state` and `simulated` accepted? Who generates the event ID (device scheme implemented; UUID may be required)?
- `timestamp_utc`: integer epoch (Technical Lead) versus ISO text (sample). `gnss_fix_quality`: integer versus text. Device ID format.
- Last-known location in events (with age) versus the null-when-no-fix packet rule: needs a decision.
- Time source for events when there is no valid GNSS time.
- Does an accepted event start as NEW on the backend, and what counts as an acknowledgement?
- Retry policy: backoff values, attempt limit (none now), queue size, drop policy when full, maximum age of queued events, ordering and priority between SOS and accident events.
- Flash: real storage choice, wear, power-loss safety (only a RAM model exists).
- Does EVENT_ACTIVE end on acknowledgement or after a hold time? Is the state entered for events created during start-up?
- Manual SOS: does the button exist, its pin, circuit and protection (PCB), hold time, cooldown, feedback (LED, buzzer), cancel option.
- Full Stack integration (endpoint, acceptance and dashboard check) not done yet.
