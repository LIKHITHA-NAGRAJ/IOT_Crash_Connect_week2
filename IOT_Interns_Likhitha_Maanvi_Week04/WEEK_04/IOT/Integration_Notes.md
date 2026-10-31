# Integration notes - who did what

| Module | Source | Status |
|---|---|---|
| GNSS parsing, last valid fix, NO_FIX/STALE, UTC | Likhitha (from Week 2 module, extended) | tested on recorded and generated NMEA |
| State machine (events allowed while acquiring GPS, connecting, degraded) | Likhitha (extended from Week 2) | tested |
| Config, provisioning, key-value storage, clock, reset reason, watchdog | Likhitha | tested on PC with simulated storage and watchdog |
| Status message, telemetry, event builders, adaptive rate | Likhitha | tested |
| Accident detector, replay tool, scenario generator | Likhitha | tested on synthetic data |
| SOS long press and debounce | Likhitha | tested with simulated button |
| Offline queue, retry, ACK, backoff in firmware (`SOS_Events/msg_queue`, `tx_manager`, `Modem/modem`) | Likhitha, implementing the design Maanvi simulated | tested with mock modem and server |
| IMU ring buffer, low-pass filter, CSV format | Likhitha. Maanvi's Week 3 IMU/logging work was not supplied, so this is not her work | tested |
| `Offline_Buffering_Maanvi/` Python buffer, reconnect and latency simulations and her test reports | Maanvi, unchanged | re-run, passing |
| Status-field validation checklist | Requested from Maanvi; not received. A draft is in `Status_Field_Checklist.md` for her to review and correct | pending her review |

Maanvi's design values were adopted: bounded FIFO, default capacity 50, preserved capture timestamps, backoff 1, 2, 4, 8, 16, 30, 30 s, event removed only after ACK, same event id on every retry. Differences, flagged for her and the team: event lane drops the oldest record (hers drops the newest); ids and timestamps follow the agreed contract (her scripts use `evt-001` and ISO strings); her latency timer starts after the event is created, the firmware demo measures from the SOS trigger to the mock ACK.

Her files were not edited. Both of us should review the integrated project together before it is sent, as the task asks. That review has not happened yet.
