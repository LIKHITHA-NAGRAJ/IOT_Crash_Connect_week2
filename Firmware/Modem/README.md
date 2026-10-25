# LTE modem module (SIMCom A7670C)

**Files:** `modem.h`, `modem.cpp` (mock bring-up state machine), `comms.h`, `comms.cpp` (Week 2 stub, kept for the legacy test)
**Documents:** `LTE_Bringup_Checklist.md` (sequence, expected responses, failure states), `Modem_Data_Approach.md` (modem TCP/MQTT versus PPP, recommendation pending Technical Lead confirmation)
**Does:** power-up, AT check, SIM check, network registration, signal quality, APN configuration and data session, with retries, timeouts and distinct failure states.
**Tested with:** MOCK AT responses only, 14 checks in `Tests/Test_Suites/tests_modem.cpp`. `READY_SIMULATED` is NOT a real registration or data session.
**Not done:** real UART transport, real AT-command timing and syntax (to verify against the A76XX AT manual), real SIM, APN, antenna or power behaviour.
