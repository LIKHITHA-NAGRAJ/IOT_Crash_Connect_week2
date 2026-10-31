# Error code table

One table (`Firmware/Common/errors.h`) used by every module through `LOGW(tag, code, ...)` and `LOGE(tag, code, ...)`. Log lines print `E<number> <NAME>`.

Log levels: ERROR (an operation failed or data was lost), WARN (degraded but handled), INFO (state changes, events, ACKs), DEBUG (per-telemetry sends). The level is stored as `log_level` (0 none, 1 error, 2 warn, 3 info, 4 debug), default 3.

| Code | Name | Severity | Meaning |
|---|---|---|---|
| E0 | ERR_NONE | INFO | No error |
| E100 | ERR_CONFIG_INVALID | CRITICAL | Stored configuration is unusable |
| E101 | ERR_CONFIG_DEFAULTED | WARNING | A configuration value was missing or out of range, default used |
| E102 | ERR_STORAGE_FAILED | ERROR | Non-volatile storage read or write failed |
| E200 | ERR_GNSS_NO_DATA | WARNING | No valid NMEA data received from the GNSS receiver |
| E201 | ERR_GNSS_NO_FIX | WARNING | GNSS receiver reports no position fix |
| E202 | ERR_GNSS_STALE | WARNING | GNSS data stopped arriving |
| E203 | ERR_GNSS_CHECKSUM | WARNING | NMEA sentence rejected, checksum mismatch |
| E300 | ERR_IMU_UNAVAILABLE | ERROR | IMU is not available |
| E301 | ERR_IMU_SAMPLE_INVALID | WARNING | IMU sample invalid and ignored |
| E400 | ERR_MODEM_NO_RESPONSE | ERROR | Modem does not answer AT commands |
| E401 | ERR_MODEM_SIM_NOT_READY | ERROR | SIM card not ready |
| E402 | ERR_MODEM_NOT_REGISTERED | WARNING | Modem not registered on the network |
| E403 | ERR_MODEM_DATA_SESSION | ERROR | Data session could not be set up |
| E404 | ERR_MODEM_AT_ERROR | ERROR | Modem returned ERROR or an unexpected reply |
| E500 | ERR_TX_NO_NETWORK | WARNING | Transmission waiting, no network |
| E501 | ERR_TX_NO_SERVER | WARNING | Server unreachable or no HTTP answer, will retry |
| E502 | ERR_TX_REJECTED | ERROR | Server rejected the payload, record dropped |
| E503 | ERR_TX_ACK_TIMEOUT | WARNING | No acknowledgement within the timeout, will retry |
| E600 | ERR_QUEUE_DROP_NEWEST | WARNING | Queue full, newest record dropped |
| E601 | ERR_QUEUE_DROP_OLDEST | WARNING | Queue full, oldest record dropped |
| E602 | ERR_PAYLOAD_TOO_LARGE | ERROR | Payload does not fit its buffer, not sent |
| E700 | ERR_SELF_TEST_FAILED | CRITICAL | Start-up self-test failed |
| E701 | ERR_WATCHDOG_RESET | CRITICAL | Device restarted by the watchdog |
| E800 | ERR_TIME_UNAVAILABLE | WARNING | No trusted UTC time available |
