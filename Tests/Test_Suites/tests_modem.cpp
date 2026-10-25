#include <Arduino.h>
#include <string.h>
#include "config.h"
#include "modem.h"
#include "tests.h"

// LTE modem bring-up tests using a MOCK modem.
// Every response comes from software. Nothing here talks to a real A7670C, and READY_SIMULATED
// is NOT a real network registration, data session, or transmission.

static int passed = 0, failed = 0;

static void check(bool ok, const char* label) {
  Serial.print(ok ? "  PASS: " : "  FAIL: ");
  Serial.println(label);
  if (ok) passed++; else failed++;
}

// Runs the state machine until it reaches a terminal state or the simulated time limit.
static Modem::Status runScenario(const char* apn, Modem::MockScenario sc, uint32_t limitMs) {
  Modem::begin(apn, sc);
  Modem::powerOn();
  uint32_t t0 = millis();
  while (!Modem::isTerminal() && millis() - t0 < limitMs) {
    Modem::step();
    delay(100);
  }
  return Modem::getStatus();
}

int runModemTests() {
  Serial.println("\n=== MODEM WEEK 3 TESTS (MOCK responses, no hardware) ===");
  const char* TEST_APN = "mock.apn.test";   // made-up test value, not a real APN
  Modem::Status s;

  Serial.println("\nTest 1: everything works (mock)");
  s = runScenario(TEST_APN, Modem::MOCK_ALL_OK, 60000);
  check(s.state == Modem::READY_SIMULATED, "reaches READY_SIMULATED");
  check(s.simReady && s.registered && s.dataSession, "SIM ready, registered, data session flags (all mock)");
  check(s.rssi == 18 && !s.signalWeak, "signal quality read (mock CSQ 18)");
  check(s.simulated, "status is flagged simulated");

  Serial.println("\nTest 2: modem never answers AT");
  s = runScenario(TEST_APN, Modem::MOCK_NO_RESPONSE, 60000);
  check(s.state == Modem::FAILED_NO_RESPONSE, "FAILED_NO_RESPONSE after the retry limit");
  check(s.attempts == CONFIG.comms.maxRetries, "exactly maxRetries attempts were made");

  Serial.println("\nTest 3: SIM not inserted");
  s = runScenario(TEST_APN, Modem::MOCK_NO_SIM, 60000);
  check(s.state == Modem::FAILED_NO_SIM, "FAILED_NO_SIM");

  Serial.println("\nTest 4: registration denied");
  s = runScenario(TEST_APN, Modem::MOCK_REG_DENIED, 60000);
  check(s.state == Modem::FAILED_REG_DENIED, "FAILED_REG_DENIED");

  Serial.println("\nTest 5: network never registers (timeout)");
  s = runScenario(TEST_APN, Modem::MOCK_REG_NEVER, 60000);
  check(s.state == Modem::FAILED_REG_TIMEOUT, "FAILED_REG_TIMEOUT after the registration timeout");

  Serial.println("\nTest 6: weak signal (policy is a placeholder)");
  s = runScenario(TEST_APN, Modem::MOCK_WEAK_SIGNAL, 60000);
  check(s.signalWeak && s.rssi == 3, "weak signal flagged (CSQ 3)");
  check(s.state == Modem::READY_SIMULATED, "bring-up continues (policy PENDING CONFIRMATION)");

  Serial.println("\nTest 7: data session cannot be opened");
  s = runScenario(TEST_APN, Modem::MOCK_DATA_FAIL, 60000);
  check(s.state == Modem::FAILED_DATA_SESSION, "FAILED_DATA_SESSION");
  check(!s.dataSession, "no data session flag");

  Serial.println("\nTest 8: APN still PENDING (as in config)");
  s = runScenario(CONFIG.comms.apn, Modem::MOCK_ALL_OK, 60000);
  check(s.state == Modem::FAILED_APN_PENDING, "FAILED_APN_PENDING, no invented APN used");

  Serial.print("\nRESULT: "); Serial.print(passed); Serial.print(" passed, ");
  Serial.print(failed); Serial.println(" failed");
  Serial.println("NOTE: MOCK modem only. No real registration, data session or transmission.");
  return failed;
}
