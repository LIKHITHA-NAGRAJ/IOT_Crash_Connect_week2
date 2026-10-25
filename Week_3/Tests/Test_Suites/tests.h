#pragma once

// Test suites. Each returns the number of FAILED checks (0 = all passed).
// GNSS: RECORDED sample NMEA, parser tests only.
// Modem: MOCK AT responses only.
// Integration: recorded NMEA + MOCK modem drive the state machine.
// Telemetry (legacy): Week 2 NESTED draft packet, NOT aligned to the agreed flat schema.
// None of this is physical hardware validation.

int runGnssTests();
int runModemTests();
int runIntegrationTests();
int runEventTests();             // SOS button, event payloads, retry, offline queue (simulated)
int runAccidentTests();          // simulated accident-detection replay and pipeline tests
int runTelemetryLegacyTests();   // prints results for manual review; always returns 0
