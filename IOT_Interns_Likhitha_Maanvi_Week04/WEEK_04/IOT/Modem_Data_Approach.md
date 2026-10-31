# Modem data approach (A7670C) - recommendation, NOT hardware-verified

## Options
| Option | For | Against |
|---|---|---|
| HTTP(S) POST using the modem's built-in AT HTTP commands | backend is FastAPI with JWT (per Full Stack); the HTTP status code is a natural acknowledgement; no TCP/IP stack on the ESP32; simplest retry model | one request per message, more overhead than MQTT; TLS certificates and JWT header handling inside the modem still to be solved |
| MQTT with the modem's AT MQTT commands | light, keeps a session | needs a broker, which has not been agreed; acknowledgement is at protocol level only |
| PPP on the ESP32 | full IP stack, any library | heavy for the ESP32, TLS load on the MCU, more failure modes |

**Recommendation: HTTP(S) POST via AT commands**, because it matches the backend and gives an ACK for the retry logic. To be confirmed with Puneeth. The firmware v0.4 implements this path against a mock modem.

## AT sequence implemented (all MOCK-tested)
`AT`, `ATE0`, `AT+CPIN?` (expects `+CPIN: READY`), `AT+CSQ`, `AT+CEREG?` (stat 1 or 5), `AT+CGDCONT=1,"IP","<apn>"`, `AT+CGACT=1,1`, then per message: `AT+HTTPINIT`, `AT+HTTPPARA="URL",...`, `AT+HTTPPARA="CONTENT","application/json"`, `AT+HTTPDATA=<len>,5000` (expects `DOWNLOAD`), payload, `AT+HTTPACTION=1` (expects `+HTTPACTION: 1,<status>,<len>`), `AT+HTTPTERM`.

The SIMCom A76XX AT command manual (V1.12) lists these commands (CPIN 6.2.2, CSQ 3.2.2, CEREG 5.2.2, CGDCONT 5.2.5, CGACT 5.2.4, HTTPINIT 16.2.1, HTTPPARA 16.2.3, HTTPDATA 16.2.7, HTTPACTION 16.2.4, HTTPTERM 16.2.2, URC 16.4). I confirmed only these section titles; I could not read the syntax pages. **Every parameter string and response format must be checked against the manual before hardware bring-up.** Not implemented: JWT/Authorization header, HTTPS certificate setup (`AT+CSSLCFG`), the real UART port (pins pending), PWRKEY pulse.

## LTE hardware-test checklist (needs the board, SIM and server)
1. A7670C powered at 3.4-4.2 V with 2.8 A peak capability, UART at 1.8 V logic (level shifting to the ESP32).
2. PWRKEY at least 50 ms, wait about 11 s for UART.
3. AT answers, SIM ready, registration, signal quality, PDP activation with the real APN.
4. HTTP POST to the real server URL; check status codes.
5. Measure trigger-to-dashboard time with Full Stack and note exact start and stop points.
6. Re-test reconnect after antenna removal and after server stop.
