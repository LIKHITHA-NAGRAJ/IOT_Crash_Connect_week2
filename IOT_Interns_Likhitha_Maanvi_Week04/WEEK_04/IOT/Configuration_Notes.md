# Configuration notes - firmware v0.4

All values below are DEFAULTS chosen by the IoT team for simulation. None is validated on hardware. They are stored in non-volatile storage under the keys shown (ESP32 `Preferences`/NVS on the device; an in-memory stand-in on the PC), validated on load, and replaced by the default when missing or out of range (warning E101).

## Reporting interval and adaptive rate
| Key | Default | Range | Meaning |
|---|---|---|---|
| rpt_move_s | 5 s | 1-3600 | telemetry interval while moving |
| rpt_still_s | 30 s | 1-3600 | telemetry interval while stationary (must be at least the moving interval) |
| rpt_event_s | 2 s | 1-600 | telemetry interval while an event is active (10 s hold after SOS or accident) |
| move_dkmh | 50 (5.0 km/h) | 5-500 | GPS speed that counts as moving; stationary below 40 % of it (2.0 km/h), hysteresis in between |

Without a GPS fix the IMU decides: standard deviation of the acceleration magnitude over the last 50 samples at or above 0.05 g means moving. Evidence: `Test_Results/demo_run_log.txt` lines `[RATE]`, and the unit tests under "Adaptive reporting rate". Changing `rpt_still_s` from 30 to 10 s at 50 s changed the next telemetry times (50 s, 60 s).

## Buffer (queue) settings
| Key | Default | Range | Meaning |
|---|---|---|---|
| q_evt_cap | 10 | 1-50 | event and status lane, sent first |
| q_tel_cap | 50 | 1-50 | telemetry lane (50 matches Maanvi's default) |

Overflow policy: telemetry lane drops the NEWEST record (Maanvi's choice, keeps a continuous older history). Event lane drops the OLDEST record (my proposal: a new SOS/accident must not be lost behind old unsent ones). Both are logged (E600, E601). **Both policies need team approval.** The queue is in RAM; it is NOT flash-backed and is lost on reset (shown in the demo after the watchdog reset).

## Retry, acknowledgement and reconnect
| Key | Default | Meaning |
|---|---|---|
| bo_init_s | 1 s | first retry/reconnect delay |
| bo_max_s | 30 s | maximum delay |
| ack_to_ms | 5000 ms | extra wait on top of the modem HTTP timeout (8 s) before an attempt is declared failed |

Delays: 1, 2, 4, 8, 16, 30, 30 ... s (same as Maanvi's `reconnect_simulation.py`). The same schedule is used for modem power-cycle retries and message retries. A record stays at the head of its lane until an HTTP 2xx. HTTP 400/413/422 drop the record (E502). The event id and payload are identical on every attempt. Timeouts in `Config/config.cpp` (`Timing`): modem boot wait 11 s, command 1 s, registration 30 s, HTTP 8 s, registration check 3 s.

## SOS, watchdog, logging
| Key | Default | Meaning |
|---|---|---|
| sos_hold_ms | 2000 | long-press time |
| sos_deb_ms | 50 | debounce |
| wd_timeout_s | 8 | watchdog timeout (simulated on the PC; ESP32 task watchdog code is untested) |
| log_level | 3 | 0 none, 1 error, 2 warn, 3 info, 4 debug |
| boot_count | 0 | incremented at every boot, part of the event id |

## Identity and server (stored, values pending)
| Key | Default | Note |
|---|---|---|
| device_id | UNPROVISIONED | set with `config_provision()`; telemetry says `provisioned:false` until set |
| apn | PENDING_CONFIRMATION | SIM/operator not known |
| server_url | PENDING_CONFIRMATION | needs Full Stack |

## Accident detector (placeholders, in `Accident_Detection/detector.cpp`)
Trigger 3.0 g for at least 20 ms, or a sharp onset (at least 100 g/s) reaching 2.0 g for 20 ms; 5 s observation window; stillness = acceleration within 1 g +/- 0.15 g and gyro below 20 dps for 1 s; rotation of at least 60 degrees from the pre-event gravity direction gives ROLLOVER_SUSPECTED; if GPS speed is available it must be at most 5 km/h at the end; 10 s lock-out after a decision. Initial values come from common vehicle-safety rules of thumb and the Week 2 test data, not from crash data.

## Error codes
See `Error_Codes.md` (25 codes, one table used by all modules).
