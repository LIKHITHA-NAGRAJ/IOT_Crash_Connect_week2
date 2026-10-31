# Module notes

Short notes on what each module is responsible for. Hardware access is stubbed or simulated unless a module says otherwise.

| Folder | Files | Responsibility |
|---|---|---|
| `crashconnect_fw.ino` | main sketch | Starts the modules, runs the main loop, reads inputs, calls the state machine, builds and sends telemetry |
| `src/common` | `cc_types.h` | Shared enums: device states and event types, and their names |
| `src/config` | `config.h/.cpp` | All settings in one structure (device ID, comms, timing, thresholds, pins). Values are drafts, unknown ones are `PENDING_CONFIRMATION` or -1 |
| `src/gnss` | `gnss.h/.cpp` | NMEA text in, checksum check, GGA/RMC decoding with TinyGPS++ (GP and GN talkers). Reports a fix only if valid and recent. UART read is a stub |
| `src/imu` | `imu.h/.cpp` | Motion samples and acceleration magnitude. Simulated test values now. `imu_hw_read_lsm6dsox()` is a stub for the real sensor |
| `src/comms` | `comms.h/.cpp` | Cellular link and sending. Stub: logs what it would send. A7670C AT commands, MQTT/API pending |
| `src/safety` | `safety_event.h/.cpp` | Decides IMPACT_SUSPECTED or SOS. Placeholder rule, not accident detection |
| `src/sos` | `sos.h/.cpp` | SOS button. Simulated input now, real pin pending |
| `src/power` | `power.h/.cpp` | Battery voltage. Stub, always unavailable |
| `src/fault` | `fault.h/.cpp` | Fault flags and whether they are critical |
| `src/status` | `device_status.h/.cpp` | One summary of module health and a status line for the log |
| `src/state` | `device_state.h/.cpp` | State machine: BOOT, SELF_TEST, ACQUIRING_GPS, NETWORK_CONNECTING, OPERATIONAL, EVENT_ACTIVE, DEGRADED, FAULT |
| `src/telemetry` | `telemetry.h/.cpp` | Builds the draft JSON packet; unavailable values are `null` |
| `src/sim` | `sim_scenario.h/.cpp` | Test only: feeds recorded NMEA and test values on a timetable. All its output is labelled SIMULATED |

Library needed: TinyGPSPlus (see `libraries.txt`).
