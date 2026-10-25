# SOS and event handling

**Files:** `sos_button.*` (long press, debounce, feedback, cooldown), `event_builder.*` (event JSON), `event_queue.*` (offline queue modelling flash storage), `event_transport.*` (retry, acknowledgement, MOCK transport and backend), `event_manager.*` (creates events with time and location rules)
**Documents:** `Event_Model_and_Transmission.md` (fields, rules, queue design, limits), `Event_API_Test_Cases.md` (checklist for Full Stack)
**Tested with:** SIMULATED button signal, mock transport and backend, simulated flash and clock: 56 checks in `Tests/Test_Suites/tests_events.cpp`. Results: `Tests/Evidence/Event_Retry_Duplicate_Results.md`.
**Test data:** `Test_Data/Sample_Events/` (12 mock valid and invalid payloads).
**NOT done:** sending an event to the real backend and checking the dashboard (needs the endpoint and confirmed contract). No physical button, modem or flash was used. The extra event fields and the timestamp, fix-quality and location rules are unconfirmed.
