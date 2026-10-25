# LTE Modem Bring-Up: State and Response Checklist (A7670C)

> **MOCK ONLY.** Every response below was produced by a software mock (`modem.cpp`), not by a real A7670C.
> The AT commands are examples to be **verified against the SIMCom A76XX AT command manual** before real use.
> `READY_SIMULATED` does NOT mean real network registration, a real data session, or real transmission.

## Expected sequence
| # | State | Action (example AT command, verify in manual) | Expected response | Failure state | Retry / timeout |
|---|---|---|---|---|---|
| 1 | POWER_UP | Power-up (PWRKEY pulse, pin PENDING with PCB), wait `powerUpMs` | modem boots | none | wait only |
| 2 | AT_CHECK | `AT` | `OK` | FAILED_NO_RESPONSE | `maxRetries` attempts, `retryDelayMs` apart |
| 3 | SIM_CHECK | `AT+CPIN?` | `+CPIN: READY` | FAILED_NO_SIM | `maxRetries` attempts |
| 4 | REGISTERING | `AT+CEREG?` (LTE registration) | status 1 (home) or 5 (roaming) | FAILED_REG_DENIED (status 3), FAILED_REG_TIMEOUT | poll every `retryDelayMs` until `registerTimeoutMs` |
| 5 | SIGNAL_CHECK | `AT+CSQ` | `+CSQ: <rssi>,<ber>` | none (weak signal is only flagged; policy PENDING) | single read |
| 6 | APN_CONFIG | `AT+CGDCONT=1,"IP","<apn>"` | `OK` | FAILED_APN_PENDING (APN not configured), FAILED_DATA_SESSION | `maxRetries` attempts |
| 7 | DATA_SESSION | start the modem's data service (example `AT+NETOPEN`; depends on the TCP/MQTT versus PPP decision) | success code | FAILED_DATA_SESSION | `maxRetries` attempts |
| 8 | READY_SIMULATED | mock "ready" | (terminal) | | |

## Mock test results (all PASS in the host build; to be re-run in Wokwi)
| Scenario | Expected end state |
|---|---|
| All OK | READY_SIMULATED |
| Modem silent | FAILED_NO_RESPONSE after 3 attempts |
| SIM not inserted | FAILED_NO_SIM |
| Registration denied | FAILED_REG_DENIED |
| Never registers | FAILED_REG_TIMEOUT |
| Weak signal | flagged, continues (policy PENDING) |
| Data session fails | FAILED_DATA_SESSION |
| APN still PENDING | FAILED_APN_PENDING (no APN is invented) |

## Not covered (needs real hardware)
Real UART timing and baud rate, power-up timing, PWRKEY/RESET behaviour, URCs, SIM and operator behaviour, real registration times, signal values, APN, antenna performance, power draw.

## Settings (placeholders in `config`)
`powerUpMs`, `registerTimeoutMs`, `minRssi`, plus `maxRetries` and `retryDelayMs`. All PENDING CONFIRMATION.
