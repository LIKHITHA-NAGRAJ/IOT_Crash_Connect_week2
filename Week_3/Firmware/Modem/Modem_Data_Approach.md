# Modem Data Approach: Modem TCP/MQTT versus PPP Client

> Recommendation only. **PENDING Technical Lead confirmation.** No dependent module should be built until it is confirmed.
> Nothing here was tested on real hardware, and no real network connection was established.

## Options
| | A. Modem-internal TCP / MQTT (AT-command services) | B. PPP client on the ESP32 |
|---|---|---|
| How it works | The A7670C runs the IP stack (and, per its AT manual, TCP/MQTT/HTTP services). The ESP32 sends AT commands and payloads. | The modem gives a raw IP link over UART (PPP). The ESP32 runs the IP stack and uses standard TCP, MQTT or HTTP libraries. |
| ESP32 load | Low: no PPP or IP stack on the ESP32 | Higher: PPP and lwIP on the ESP32 (RAM and CPU) |
| Code reuse | Modem-specific AT commands | Standard network libraries, portable to other modems |
| Failure modes | AT parsing, URCs, modem MQTT firmware limits | PPP negotiation, mode switching, harder to monitor the modem while in data mode |
| Monitoring registration, signal | Easy: AT commands stay available | Harder: the UART carries PPP data, so AT needs command mode |
| TLS | Depends on modem support (verify in the AT manual) | ESP32 handles TLS (more RAM) |
| Dependencies | A76XX AT command manual | PPP support in the chosen framework (verify for Arduino versus ESP-IDF) |

## Recommendation (for confirmation)
**Option A: modem-internal TCP/MQTT.**

**Reasons**
- Telemetry is small (one short JSON message every few seconds, plus event messages), so a full IP stack on the ESP32 is not needed.
- It keeps the ESP32 free for sensors, the state machine and event handling.
- Registration and signal quality remain easy to monitor with AT commands, which fits the bring-up state machine already built.
- It is the simpler path for a first working version.

**Assumptions**
- The A7670C supports the needed services (TCP and/or MQTT, and TLS if required) and they behave as the manual describes.
- The backend accepts the chosen transport. The MQTT versus REST decision is still open.
- Message size is small and send rate is low.

**Dependencies**
- Technical Lead decision: MQTT or REST, broker or API address, authentication, TLS requirement.
- SIM, APN and data plan from the operator.
- A76XX AT command manual for exact syntax.

**Limitations**
- Tied to SIMCom AT commands, so changing modem later means rewriting this layer.
- Modem MQTT features (QoS, keep-alive, reconnect) may be limited. To verify.
- Cannot be tested without hardware, so only the mock state machine is tested.
- PPP would be the better choice if the project needs standard libraries, a different transport (for example HTTPS to arbitrary servers), or modem independence. That would cost more RAM and complexity.

## What does not depend on this decision
GNSS module, state machine, telemetry builder, and the modem bring-up up to a data session. Only the transport layer behind `DATA_SESSION` depends on it.
