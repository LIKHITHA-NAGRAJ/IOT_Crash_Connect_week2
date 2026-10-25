#include <Arduino.h>
#include <string.h>
#include "config.h"
#include "gnss.h"
#include "modem.h"
#include "state_machine.h"
#include "device_integration.h"
#include "tests.h"

// Integration tests: GNSS fix and MOCK modem status drive the device state machine.
// GPS input = RECORDED sample NMEA lines. Modem = MOCK AT responses. Time is the simulator's.
// None of this is physical hardware validation, and READY_SIMULATED is not a real LTE connection.

static int passed = 0, failed = 0;
static void check(bool ok, const char* label) {
  Serial.print(ok ? "  PASS: " : "  FAIL: ");
  Serial.println(label);
  if (ok) passed++; else failed++;
}

static const char* GGA_OK    = "$GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47";
static const char* RMC_OK    = "$GPRMC,123519,A,4807.038,N,01131.000,E,022.4,084.4,230394,003.1,W*6A";
static const char* GGA_NOFIX = "$GPGGA,123520,,,,,0,00,,,M,,M,,*61";
static const char* RMC_NOFIX = "$GPRMC,123520,V,,,,,,,230394,,,N*5B";
static const char* TEST_APN  = "mock.apn.test";   // made-up test value, not a real APN

enum GpsMode { GPS_AUTO, GPS_VALID, GPS_NOFIX, GPS_SILENT };
static uint32_t lastFeed = 0;

static void resetAll(Modem::MockScenario sc) {
  GNSS::begin();
  Modem::begin(TEST_APN, sc);
  DeviceState::begin();
  Modem::powerOn();
  lastFeed = millis() - 1000;
}

// Runs the system for ms of simulated time in 100 ms ticks.
static void run(uint32_t ms, GpsMode mode) {
  uint32_t t0 = millis();
  while (millis() - t0 < ms) {
    if (millis() - lastFeed >= 1000) {
      lastFeed = millis();
      if (mode == GPS_AUTO) GNSS::update();
      else if (mode == GPS_VALID) { GNSS::feedLine(GGA_OK); GNSS::feedLine(RMC_OK); }
      else if (mode == GPS_NOFIX) { GNSS::feedLine(GGA_NOFIX); GNSS::feedLine(RMC_NOFIX); }
      // GPS_SILENT: nothing is fed
    }
    Modem::step();
    DeviceState::step(Integration::makeInputs(true, false, false));
    delay(100);
  }
}

static void showStatus() {
  char buf[120];
  Integration::statusText(buf, sizeof buf);
  Serial.print("  status: "); Serial.println(buf);
}

int runIntegrationTests() {
  Serial.println("\n=== INTEGRATION TESTS (recorded NMEA + MOCK modem) ===");

  Serial.println("\nTest 1: normal start-up");
  resetAll(Modem::MOCK_ALL_OK);
  run(20000, GPS_AUTO);
  showStatus();
  check(DeviceState::get() == DeviceState::OPERATIONAL, "reaches OPERATIONAL (mock network)");

  Serial.println("\nTest 2: GPS loss then recovery (while OPERATIONAL)");
  run(3000, GPS_NOFIX);
  showStatus();
  check(DeviceState::get() == DeviceState::DEGRADED, "GPS lost -> DEGRADED");
  run(3000, GPS_VALID);
  showStatus();
  check(DeviceState::get() == DeviceState::OPERATIONAL, "GPS recovered -> OPERATIONAL");

  Serial.println("\nTest 3: GPS data stops (last fix goes stale)");
  run(5000, GPS_SILENT);
  showStatus();
  check(DeviceState::get() == DeviceState::DEGRADED, "stale fix is not current -> DEGRADED");

  Serial.println("\nTest 4: modem fails (registration denied, mock), GPS fine");
  resetAll(Modem::MOCK_REG_DENIED);
  run(30000, GPS_VALID);
  showStatus();
  check(Modem::getStatus().state == Modem::FAILED_REG_DENIED, "modem ends in FAILED_REG_DENIED (mock)");
  check(DeviceState::get() == DeviceState::DEGRADED, "network failure -> DEGRADED");

  Serial.println("\nTest 5: GPS never delivers a fix (acquire timeout)");
  resetAll(Modem::MOCK_ALL_OK);
  run(CONFIG.timeouts.gpsAcquireMs + 2000, GPS_SILENT);
  showStatus();
  check(DeviceState::get() == DeviceState::DEGRADED, "GPS acquire timeout -> DEGRADED (PENDING CONFIRMATION)");

  Serial.println("\nTest 6: self-test fails");
  DeviceState::begin();
  DeviceState::Inputs bad = {};
  bad.selfTestOk = false;
  DeviceState::step(bad);
  DeviceState::step(bad);
  check(DeviceState::get() == DeviceState::FAULT, "SELF_TEST failure -> FAULT");

  Serial.print("\nRESULT: "); Serial.print(passed); Serial.print(" passed, ");
  Serial.print(failed); Serial.println(" failed");
  Serial.println("NOTE: recorded GPS data + MOCK modem. No real GPS, LTE registration or transmission.");
  return failed;
}
