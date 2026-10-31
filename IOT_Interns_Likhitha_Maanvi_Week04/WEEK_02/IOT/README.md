> **SUPERSEDED FORMAT:** the draft packet in this Week 2 folder is NESTED (location, motion sub-objects) and used an ISO timestamp. It predates the agreed flat telemetry format. The current flat format is in ../../WEEK_04/IOT/ (see Contract_and_Pending_Issues.md).

# CRASH CONNECT - IoT Week 2 Deliverable

**Intern:** Likhitha N | **Role:** IoT / Embedded | **Company:** CRASH CONNECT PVT LTD
Company-internal, please do not share outside the team.

> **SIMULATED / TEST DATA.** GPS = recorded NMEA sentences, not live GPS. IMU, SOS button and network are test inputs.
> Hardware access (GNSS UART, LSM6DSOX, A7670C, SOS pin, battery ADC) is a **stub**. Unknown decisions are marked **Pending Confirmation**.

## Week 2 topic
Vehicle product architecture: where the ESP32-S3 firmware fits, a modular firmware project, the device state machine and a first telemetry packet builder.

| Day | Work | Document |
|---|---|---|
| Monday | System flow, firmware module list, device-state diagram, power block diagram, component notes | `documentation/Day1_Firmware_Architecture.pdf` |
| Tuesday | ESP32-S3 project with module folders, headers, stubs, configuration structure, module notes | `documentation/Day2_Project_Structure.pdf`, `firmware/crashconnect_fw/MODULES.md` |
| Wednesday | State machine with test inputs, telemetry packet builder, unavailable-value handling | `documentation/Day3_State_Machine_and_Telemetry.pdf` |
| All week | Open questions | `documentation/Pending_Confirmation.pdf` |

## Folder structure
| Folder | Contents |
|---|---|
| `firmware/crashconnect_fw/` | ESP32-S3 sketch and the module folders under `src/` |
| `firmware/host_tests/` | PC test programs and `run_tests.sh` (same source code, tiny Arduino stand-in) |
| `firmware/platformio.ini` | Optional PlatformIO project file (not tested here) |
| `sensor_data/` | Recorded NMEA samples (GP and GN talker), simulated IMU values |
| `test_results/` | Unit test output, demonstration output, summary |
| `screenshots/` | To be added from Arduino IDE or Wokwi (see the README inside) |
| `documentation/` | Day documents, pending list, diagrams |

## Development environment
- ESP32-S3, Arduino C++, Serial Monitor at 115200 baud (Week 1 setup), library TinyGPSPlus.
- Arduino IDE: open `firmware/crashconnect_fw/crashconnect_fw.ino`. The `src/` sub-folders are compiled automatically.
- PC test: `TINYGPS_SRC=<path to TinyGPSPlus/src> firmware/host_tests/run_tests.sh`

## What was built
- **Modules:** config, gnss, imu, comms, safety_event, sos, power, fault, device_status, state, telemetry, sim. Each has a header with the interface and a source file; responsibilities are in `firmware/crashconnect_fw/MODULES.md`.
- **Configuration:** one structure in `src/config/`. No hard-coded values in the logic. Unknown values are `PENDING_CONFIRMATION` or -1.
- **State machine:** BOOT, SELF_TEST, ACQUIRING_GPS, NETWORK_CONNECTING, OPERATIONAL, EVENT_ACTIVE, DEGRADED, FAULT, with the transitions in the Day 3 document. A timetable of test inputs (`src/sim/`) demonstrates all of them.
- **GNSS:** TinyGPS++ parsing with checksum check. Handles GP and GN talkers. A fix is only reported when it is valid and recent, and an old position is never reused after a no-fix sentence.
- **Telemetry packet builder:** draft JSON packet (version `0.1-DRAFT`). Unavailable values are `null`, not 0. A packet that does not fit the buffer is not sent.

## Tests and build status
- 66 unit tests passed, 0 failed (state transitions, fault flags, event rule, GNSS cases, telemetry packets).
- The demonstration covers a start-up, an impact test, SOS, GPS loss, network loss and a critical fault with restart. It sent 20 packets to the stub, did not send 1 (network down) and rejected 1 corrupted sentence.
- All 25 printed packets are valid JSON.
- These ran on a PC with the real TinyGPS++ source. See `test_results/test_summary.md`.

## What worked
- Module structure, configuration separated from the logic, stubs clearly labelled in the output.
- State changes and unavailable-value handling behave as designed in the tests.

## What could not be tested (no hardware / not available in my workspace)
- Compiling for the ESP32-S3 with its toolchain and running in Wokwi (screenshots still to be added).
- Real GNSS, LSM6DSOX, A7670C, SOS button and battery reading.
- Sending data to a backend.

## Problems and solutions
| Problem | Solution |
|---|---|
| TinyGPS++ custom fields match the talker name exactly, so `GNGGA` from a multi-constellation receiver would be missed | Registered both `GPGGA` and `GNGGA` and added a test for the GN talker |
| TinyGPS++ keeps the last position after a "no fix" sentence (seen in Week 1) | Fix quality from GGA plus freshness check; location fields are only filled for a valid, recent fix |
| 0 is a real value (0,0 is a real place), so unknown values cannot be sent as 0 | Unavailable values are `null` in the packet |
| The agreed common packet fields were not available to me | Draft field list (`0.1-DRAFT`), kept in one file (`src/telemetry/telemetry.cpp`) so it is easy to replace |
| No hardware | Stubs for every hardware access and a clearly labelled simulation module |

## Known limitations
- The impact rule (4.0 g) is a placeholder to demonstrate a state change. It is **not** accident detection.
- No offline buffering, no real transport, no security, no persistent storage.
- Timeouts, intervals and the restart policy are draft values.
- Datasheet values come from the pages I could read (MAX-M10S from an advance-information datasheet). They should be checked against the final datasheets.

## Pending Confirmation
22 open questions with the current assumption for each are in `documentation/Pending_Confirmation.pdf`. The main ones: agreed packet fields, MQTT or API, GNSS and IMU interfaces, pin map, 1.8 V modem logic level, modem power, backup power switching.

## Week 3 recommendations and expectations
Recommendations:
1. Replace the draft packet with the agreed fields and add the event message.
2. Implement the GNSS UART read for the MAX-M10S and the LSM6DSOX register read (start with its WHO_AM_I check) once hardware or a simulator is available.
3. Start the A7670C AT-command bring-up and the chosen transport (MQTT or API), with an offline buffer.
4. Add debounce and long-press handling for the SOS button.
5. Design impact detection properly (filtering, threshold, confirmation time) with real or recorded data.

Expectations:
- The agreed packet and event format, the transport and the pin map confirmed.
- Hardware access, or confirmation that simulation should continue.
- Guidance on the restart behaviour and the timeouts.
