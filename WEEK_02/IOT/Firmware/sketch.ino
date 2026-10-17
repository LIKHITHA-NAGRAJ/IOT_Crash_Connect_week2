#include "config.h"
#include "gnss.h"
#include "imu.h"
#include "comms.h"
#include "state_machine.h"
#include "telemetry.h"

// All tests below use RECORDED / SIMULATED / TEST inputs. No GPS, IMU or modem
// hardware is used, so none of this validates real hardware behaviour.

static void showPacket(const GNSS::Fix& fix, bool imuOk, const IMU::Reading& r, bool eventActive) {
  Telemetry::Data d = {
    fix.valid ? fix.time : nullptr,   // timestamp only when the GPS fix is valid
    fix.valid,
    fix.lat,
    fix.lon,
    imuOk,
    r,
    eventActive
  };
  char json[512];
  if (Telemetry::buildJson(d, json, sizeof(json))) {
    Serial.println(json);
    Serial.println("Packet build: SUCCESS (NOT sent, no modem)");
  } else {
    Serial.println("Packet build: FAILED (buffer too small or formatting error)");
  }
}

// Helper: reach OPERATIONAL using simulated "everything OK" inputs.
static void goToOperational() {
  DeviceState::begin();
  DeviceState::Inputs ok = { true, true, true, false, false };
  for (int i = 0; i < 4; i++) DeviceState::step(ok);
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n=== CRASH CONNECT: SIMULATED TESTS (no hardware) ===");
  Serial.print("Device ID: "); Serial.println(CONFIG.deviceId);
  Serial.print("Simulation mode: ");
  Serial.println(CONFIG.simulateHardware ? "YES" : "NO");

  GNSS::begin();
  IMU::begin();
  Comms::begin();
  Comms::connectNetwork();   // stub: always NOT connected. 'networkUp' below is a SIMULATED condition.

  IMU::Reading still = IMU::readData();   // stationary TEST values

  // ---------------------------------------------------------------
  Serial.println("\n--- TEST 1: normal start-up (GPS from recorded NMEA, network = simulated) ---");
  DeviceState::begin();
  for (int i = 0; i < 9; i++) {
    DeviceState::Inputs in = {};
    in.selfTestOk = true;
    if (i >= 2) GNSS::update();               // feed one recorded NMEA line
    GNSS::Fix fix;
    in.gpsFix = GNSS::getFix(fix);
    in.networkUp = (i >= 6);                  // SIMULATED: network comes up at step 6
    DeviceState::step(in);
  }
  GNSS::Fix fix1;
  GNSS::getFix(fix1);
  showPacket(fix1, true, still, false);

  // ---------------------------------------------------------------
  Serial.println("\n--- TEST 2: no GPS fix -> location and timestamp must be null ---");
  GNSS::begin();                              // fresh module, no fix yet
  GNSS::Fix noFix;
  GNSS::getFix(noFix);
  showPacket(noFix, true, still, false);

  Serial.println("\n--- TEST 3: IMU unavailable -> IMU values must be null ---");
  showPacket(fix1, false, still, false);

  // ---------------------------------------------------------------
  Serial.println("\n--- TEST 4: movement burst -> EVENT_ACTIVE (SIMULATED test values) ---");
  goToOperational();
  IMU::Reading burst = { 1.8f, 1.2f, 1.2f, 90.0f, 60.0f, 40.0f };   // TEST values
  float mag = IMU::accelerationMagnitude(burst);
  Serial.print("Acceleration magnitude (g): "); Serial.println(mag, 3);
  Serial.print("Placeholder threshold (g): "); Serial.println(CONFIG.event.crashThresholdG, 2);
  DeviceState::Inputs ev = { true, true, true, false, false };
  ev.eventDetected = (mag >= CONFIG.event.crashThresholdG);
  DeviceState::step(ev);                      // OPERATIONAL -> EVENT_ACTIVE
  showPacket(fix1, true, burst, true);
  ev.eventDetected = false;
  ev.eventCleared = true;
  DeviceState::step(ev);                      // EVENT_ACTIVE -> OPERATIONAL

  // ---------------------------------------------------------------
  Serial.println("\n--- TEST 5: GPS fix lost -> DEGRADED, then recovery ---");
  goToOperational();
  DeviceState::Inputs lost = { true, false, true, false, false };
  DeviceState::step(lost);                    // OPERATIONAL -> DEGRADED
  DeviceState::Inputs back = { true, true, true, false, false };
  DeviceState::step(back);                    // DEGRADED -> OPERATIONAL

  // ---------------------------------------------------------------
  Serial.println("\n--- TEST 6: self-test fails -> FAULT; illegal transition rejected ---");
  DeviceState::begin();
  DeviceState::Inputs bad = { false, true, true, false, false };
  DeviceState::step(bad);                     // BOOT -> SELF_TEST
  DeviceState::step(bad);                     // SELF_TEST -> FAULT
  DeviceState::transitionTo(DeviceState::OPERATIONAL);   // must be rejected

  Serial.println("\nNOTE: GPS = recorded NMEA sample (not live), IMU = test values,");
  Serial.println("network = simulated condition. No cellular transmission occurred.");
}

void loop() {
  // Periodic processing will be added later.
}
