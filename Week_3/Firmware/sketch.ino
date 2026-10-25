#include "config.h"
#include "tests.h"

// Week 3 firmware test runner (Wokwi, ESP32-S3).
// Choose which suites to run. The integration suite simulates about two minutes of time.
//
// ALL INPUTS ARE RECORDED, SIMULATED OR MOCKED. A pass here does NOT validate real GPS,
// IMU, modem or network hardware, and READY_SIMULATED is not a real LTE connection.

static const bool RUN_GNSS            = true;   // recorded NMEA parser tests
static const bool RUN_MODEM           = true;   // mock AT-command modem tests
static const bool RUN_INTEGRATION     = true;   // GNSS + mock modem -> state machine
static const bool RUN_ACCIDENT        = true;   // staged accident detector replay (simulated samples)
static const bool RUN_EVENTS          = true;   // SOS, event payloads, retry and offline queue (simulated)
static const bool RUN_TELEMETRY_OLD   = true;   // Week 2 nested draft packet (legacy)

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n##### CRASH CONNECT IoT - WEEK 3 TEST RUN (simulated, no hardware) #####");
  Serial.print("Device ID: "); Serial.println(CONFIG.deviceId);
  Serial.print("Simulation mode: "); Serial.println(CONFIG.simulateHardware ? "YES" : "NO");

  int failed = 0;
  if (RUN_GNSS)          failed += runGnssTests();
  if (RUN_MODEM)         failed += runModemTests();
  if (RUN_INTEGRATION)   failed += runIntegrationTests();
  if (RUN_ACCIDENT)      failed += runAccidentTests();
  if (RUN_EVENTS)        failed += runEventTests();
  if (RUN_TELEMETRY_OLD) failed += runTelemetryLegacyTests();

  Serial.println("\n##### SUMMARY #####");
  Serial.print("Failed checks across self-checking suites: "); Serial.println(failed);
  Serial.println("Recorded GPS data + mock modem + simulated sensor values only.");
}

void loop() {
}
