# Status message field checklist (DRAFT by Likhitha for Maanvi to review)

| Field | Source | Unavailable behaviour | Checked by tests |
|---|---|---|---|
| device_id | stored configuration | "UNPROVISIONED" and `provisioned:false` | yes |
| firmware_version | build constant "0.4.0" | - | yes |
| uptime_s | device clock | - | yes |
| reset_reason | ESP reset reason (host: simulated) | "UNKNOWN" | yes (TASK_WDT after watchdog) |
| boot_count | stored counter | - | yes |
| network_registered | modem state | false | yes |
| signal_csq / signal_dbm | AT+CSQ | null (also when CSQ is 99) | yes |
| power_source / battery_mv | no source defined | always null | yes |
| timestamp_utc / time_source | GNSS or simulated clock | null / "NONE" | yes |

To review: field names and types with Puneeth, whether `signal_dbm` should be derived on the device, and whether `power_source` needs a defined set of values.
