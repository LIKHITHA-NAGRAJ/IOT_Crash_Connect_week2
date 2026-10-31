#include "arduino_stub/Arduino.h"

unsigned long fake_ms = 0;
SerialClass Serial;

#include "../crashconnect_fw/src/common/cc_types.h"
#include "../crashconnect_fw/src/config/config.h"
#include "../crashconnect_fw/src/fault/fault.h"
#include "../crashconnect_fw/src/gnss/gnss.h"
#include "../crashconnect_fw/src/imu/imu.h"
#include "../crashconnect_fw/src/safety/safety_event.h"
#include "../crashconnect_fw/src/state/device_state.h"
#include "../crashconnect_fw/src/telemetry/telemetry.h"

static int passed = 0;
static int failed = 0;

#define CHECK(cond, name)                                  \
  do {                                                     \
    if (cond) { passed++; printf("PASS  %s\n", name); }    \
    else { failed++; printf("FAIL  %s\n", name); }         \
  } while (0)

static StateInputs good() {
  StateInputs in;
  memset(&in, 0, sizeof(in));
  in.self_test_pass = true;
  in.gps_fix = true;
  in.network_up = true;
  in.event = EVENT_NONE;
  return in;
}

static bool expect(DeviceState from, StateInputs in, uint32_t t, DeviceState want) {
  in.time_in_state_ms = t;
  const char *reason = 0;
  return state_next(from, in, &reason) == want;
}

static void test_state_machine() {
  printf("\n== STATE MACHINE ==\n");
  const TimingConfig &t = config().timing;
  StateInputs in;

  CHECK(expect(STATE_BOOT, good(), 0, STATE_BOOT), "BOOT stays before boot time");
  CHECK(expect(STATE_BOOT, good(), t.boot_ms, STATE_SELF_TEST), "BOOT -> SELF_TEST after boot time");

  CHECK(expect(STATE_SELF_TEST, good(), 0, STATE_SELF_TEST), "SELF_TEST stays before self-test time");
  CHECK(expect(STATE_SELF_TEST, good(), t.self_test_ms, STATE_ACQUIRING_GPS), "SELF_TEST -> ACQUIRING_GPS on pass");
  in = good(); in.self_test_pass = false;
  CHECK(expect(STATE_SELF_TEST, in, t.self_test_ms, STATE_FAULT), "SELF_TEST -> FAULT on fail");

  in = good(); in.gps_fix = false;
  CHECK(expect(STATE_ACQUIRING_GPS, in, 1000, STATE_ACQUIRING_GPS), "ACQUIRING_GPS waits without fix");
  CHECK(expect(STATE_ACQUIRING_GPS, good(), 1000, STATE_NETWORK_CONNECTING), "ACQUIRING_GPS -> NETWORK_CONNECTING on fix");
  CHECK(expect(STATE_ACQUIRING_GPS, in, t.gps_acquire_timeout_ms, STATE_DEGRADED), "ACQUIRING_GPS -> DEGRADED on timeout");

  in = good(); in.network_up = false;
  CHECK(expect(STATE_NETWORK_CONNECTING, in, 1000, STATE_NETWORK_CONNECTING), "NETWORK_CONNECTING waits without network");
  CHECK(expect(STATE_NETWORK_CONNECTING, good(), 1000, STATE_OPERATIONAL), "NETWORK_CONNECTING -> OPERATIONAL");
  CHECK(expect(STATE_NETWORK_CONNECTING, in, t.network_timeout_ms, STATE_DEGRADED), "NETWORK_CONNECTING -> DEGRADED on timeout");
  in = good(); in.gps_fix = false;
  CHECK(expect(STATE_NETWORK_CONNECTING, in, 1000, STATE_DEGRADED), "NETWORK_CONNECTING -> DEGRADED if fix lost");

  CHECK(expect(STATE_OPERATIONAL, good(), 1000, STATE_OPERATIONAL), "OPERATIONAL stays when all good");
  in = good(); in.event = EVENT_IMPACT_SUSPECTED;
  CHECK(expect(STATE_OPERATIONAL, in, 1000, STATE_EVENT_ACTIVE), "OPERATIONAL -> EVENT_ACTIVE on impact");
  in = good(); in.event = EVENT_SOS;
  CHECK(expect(STATE_OPERATIONAL, in, 1000, STATE_EVENT_ACTIVE), "OPERATIONAL -> EVENT_ACTIVE on SOS");
  in = good(); in.gps_fix = false;
  CHECK(expect(STATE_OPERATIONAL, in, 1000, STATE_DEGRADED), "OPERATIONAL -> DEGRADED on GPS loss");
  in = good(); in.network_up = false;
  CHECK(expect(STATE_OPERATIONAL, in, 1000, STATE_DEGRADED), "OPERATIONAL -> DEGRADED on network loss");
  in = good(); in.noncritical_fault = true;
  CHECK(expect(STATE_OPERATIONAL, in, 1000, STATE_DEGRADED), "OPERATIONAL -> DEGRADED on non-critical fault");

  in = good(); in.event = EVENT_SOS;
  CHECK(expect(STATE_EVENT_ACTIVE, in, t.event_hold_ms + 1000, STATE_EVENT_ACTIVE), "EVENT_ACTIVE stays while event present");
  CHECK(expect(STATE_EVENT_ACTIVE, good(), 1000, STATE_EVENT_ACTIVE), "EVENT_ACTIVE holds for minimum time");
  CHECK(expect(STATE_EVENT_ACTIVE, good(), t.event_hold_ms, STATE_OPERATIONAL), "EVENT_ACTIVE -> OPERATIONAL after hold");
  in = good(); in.network_up = false;
  CHECK(expect(STATE_EVENT_ACTIVE, in, t.event_hold_ms, STATE_DEGRADED), "EVENT_ACTIVE -> DEGRADED if system degraded");

  in = good(); in.gps_fix = false;
  CHECK(expect(STATE_DEGRADED, in, 1000, STATE_DEGRADED), "DEGRADED stays while problem remains");
  CHECK(expect(STATE_DEGRADED, good(), 1000, STATE_OPERATIONAL), "DEGRADED -> OPERATIONAL on recovery");
  in = good(); in.gps_fix = false; in.event = EVENT_SOS;
  CHECK(expect(STATE_DEGRADED, in, 1000, STATE_EVENT_ACTIVE), "DEGRADED -> EVENT_ACTIVE on SOS");

  in = good(); in.critical_fault = true;
  CHECK(expect(STATE_FAULT, in, t.fault_restart_ms + 5000, STATE_FAULT), "FAULT stays while critical fault present");
  CHECK(expect(STATE_FAULT, good(), 1000, STATE_FAULT), "FAULT waits restart delay");
  CHECK(expect(STATE_FAULT, good(), t.fault_restart_ms, STATE_BOOT), "FAULT -> BOOT after fault cleared");

  bool allToFault = true;
  in = good(); in.critical_fault = true;
  for (int s = 0; s < STATE_COUNT; s++) {
    if (s == STATE_FAULT) continue;
    if (!expect((DeviceState)s, in, 0, STATE_FAULT)) allToFault = false;
  }
  CHECK(allToFault, "critical fault -> FAULT from every other state");
}

static void test_fault_flags() {
  printf("\n== FAULT FLAGS ==\n");
  fault_clear_all();
  CHECK(!fault_is_critical() && !fault_is_noncritical(), "no faults at start");
  fault_set(FAULT_IMU);
  CHECK(fault_is_noncritical() && !fault_is_critical(), "IMU fault is non-critical");
  fault_set(FAULT_CONFIG);
  CHECK(fault_is_critical(), "config fault is critical");
  fault_clear_all();
  CHECK(fault_flags() == 0, "clear all");
}

static void test_safety_and_imu() {
  printf("\n== SAFETY / IMU ==\n");
  ImuSample s;
  memset(&s, 0, sizeof(s));
  s.valid = true;
  s.accel_mag_g = 3.99f;
  CHECK(safety_evaluate(s, false) == EVENT_NONE, "below threshold -> no event");
  s.accel_mag_g = config().threshold.impact_accel_g;
  CHECK(safety_evaluate(s, false) == EVENT_IMPACT_SUSPECTED, "at threshold -> impact suspected");
  CHECK(safety_evaluate(s, true) == EVENT_SOS, "SOS has priority over impact");
  s.valid = false;
  s.accel_mag_g = 9.0f;
  CHECK(safety_evaluate(s, false) == EVENT_NONE, "invalid IMU sample never raises impact");

  imu_init();
  ImuSample r;
  imu_read(&r);
  CHECK(r.valid && r.simulated, "simulated IMU sample is flagged simulated");
  CHECK(fabsf(r.accel_mag_g - 0.980f) < 0.001f, "stationary magnitude 0.980 g");
  imu_sim_set_case(IMU_CASE_IMPACT);
  imu_read(&r);
  CHECK(fabsf(r.accel_mag_g - sqrtf(3.8f * 3.8f + 2.9f * 2.9f + 2.6f * 2.6f)) < 0.001f, "impact test magnitude matches formula");
  CHECK(r.accel_mag_g >= config().threshold.impact_accel_g, "impact test sample is above placeholder threshold");
}

static void test_gnss() {
  printf("\n== GNSS ==\n");
  GnssFix f;
  fake_ms = 10000;
  gnss_init();
  CHECK(!gnss_get_fix(&f, fake_ms), "no data -> no fix");

  gnss_feed_nmea("$GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47\r\n");
  gnss_feed_nmea("$GPRMC,123519,A,4807.038,N,01131.000,E,022.4,084.4,230394,003.1,W*6A\r\n");
  bool ok = gnss_get_fix(&f, fake_ms);
  CHECK(ok && f.valid, "valid GP GGA+RMC -> fix");
  CHECK(fabs(f.latitude - 48.1173) < 0.0001 && fabs(f.longitude - 11.516667) < 0.0001, "latitude/longitude decoded");
  CHECK(f.time_valid && strcmp(f.timestamp, "1994-03-23T12:35:19Z") == 0, "timestamp uses 80-99 -> 19xx year rule");
  CHECK(f.satellites_valid && f.satellites == 8, "satellites decoded");

  gnss_feed_nmea("$GPGGA,123521,,,,,0,00,99.99,,,,,,*4E\r\n");
  gnss_feed_nmea("$GPRMC,123521,V,,,,,,,230394,,*38\r\n");
  ok = gnss_get_fix(&f, fake_ms);
  CHECK(!ok && !f.valid, "no-fix sentences -> fix invalid (old position not reused)");
  CHECK(f.satellites_valid && f.satellites == 0, "satellite count 0 still reported as available");

  gnss_feed_nmea("$GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47\r\n");
  gnss_feed_nmea("$GPRMC,123519,A,4807.038,N,01131.000,E,022.4,084.4,230394,003.1,W*6A\r\n");
  uint32_t failuresBefore = gnss_checksum_failures();
  gnss_feed_nmea("$GPGGA,123522,4807.050,N,01131.020,E,1,08,0.9,545.7,M,46.9,M,,*00\r\n");
  CHECK(gnss_checksum_failures() == failuresBefore + 1, "wrong checksum counted and rejected");
  ok = gnss_get_fix(&f, fake_ms);
  CHECK(ok && fabs(f.latitude - 48.1173) < 0.0001, "rejected sentence did not change position");

  fake_ms += config().timing.gnss_stale_ms + 1;
  CHECK(!gnss_get_fix(&f, fake_ms), "stale data -> fix invalid");

  fake_ms += 1000;
  gnss_feed_nmea("$GNGGA,123519,4807.038,N,01131.000,E,1,10,0.8,545.4,M,46.9,M,,*51\r\n");
  gnss_feed_nmea("$GNRMC,123519,A,4807.038,N,01131.000,E,022.4,084.4,230394,003.1,W,A*19\r\n");
  ok = gnss_get_fix(&f, fake_ms);
  CHECK(ok && f.satellites == 10, "GN talker (multi-constellation) accepted");
  gnss_feed_nmea("$GNGGA,123521,,,,,0,00,99.99,,,,,,*50\r\n");
  gnss_feed_nmea("$GNRMC,123521,V,,,,,,,230394,,,N*44\r\n");
  CHECK(!gnss_get_fix(&f, fake_ms), "GN no-fix -> fix invalid");
}

static void test_telemetry() {
  printf("\n== TELEMETRY ==\n");
  char buf[640];

  GnssFix fix;
  memset(&fix, 0, sizeof(fix));
  fix.valid = true;
  fix.fix_quality = 1;
  fix.latitude = 48.1173;
  fix.longitude = 11.516667;
  fix.altitude_valid = true; fix.altitude_m = 545.4f;
  fix.speed_valid = true; fix.speed_kmh = 41.5f;
  fix.hdop_valid = true; fix.hdop = 0.9f;
  fix.satellites_valid = true; fix.satellites = 8;
  fix.time_valid = true;
  strcpy(fix.timestamp, "1994-03-23T12:35:19Z");

  ImuSample imu;
  memset(&imu, 0, sizeof(imu));
  imu.valid = true; imu.simulated = true;
  imu.ax = 0.02f; imu.ay = 0.01f; imu.az = 0.98f;
  imu.gx = 0.5f; imu.gy = 0.2f; imu.gz = 1.0f;
  imu.accel_mag_g = 0.98f;

  TelemetryInput in;
  memset(&in, 0, sizeof(in));
  in.gnss = &fix;
  in.imu = &imu;
  in.state = STATE_OPERATIONAL;
  in.event = EVENT_NONE;
  in.comms_connected = true;
  in.battery_valid = true;
  in.battery_mv = 3900;
  in.uptime_ms = 12345;
  in.seq = 1;
  size_t n = telemetry_build_json(in, buf, sizeof(buf));
  CHECK(n > 0 && n == strlen(buf), "packet with all values built");
  CHECK(strstr(buf, "\"latitude\":48.117300") != 0, "latitude present");
  CHECK(strstr(buf, "\"battery_mv\":3900") != 0, "battery present when available");
  printf("PACKET: %s\n", buf);

  fix.valid = false;
  fix.time_valid = false;
  imu.valid = false;
  in.battery_valid = false;
  in.comms_connected = false;
  in.state = STATE_DEGRADED;
  in.seq = 2;
  n = telemetry_build_json(in, buf, sizeof(buf));
  CHECK(n > 0, "packet with unavailable values built");
  CHECK(strstr(buf, "\"latitude\":null") != 0 && strstr(buf, "\"longitude\":null") != 0, "no fix -> latitude/longitude null, not 0");
  CHECK(strstr(buf, "\"timestamp_utc\":null") != 0, "no fix -> timestamp null");
  CHECK(strstr(buf, "\"accel_magnitude_g\":null") != 0, "invalid IMU -> motion null, not 0");
  CHECK(strstr(buf, "\"battery_mv\":null") != 0, "battery unavailable -> null");
  CHECK(strstr(buf, "\"fix\":false") != 0, "fix flag false");
  printf("PACKET: %s\n", buf);

  in.gnss = 0;
  in.imu = 0;
  n = telemetry_build_json(in, buf, sizeof(buf));
  CHECK(n > 0 && strstr(buf, "\"latitude\":null") != 0, "missing GNSS/IMU pointers handled");
  printf("PACKET: %s\n", buf);

  char small[40];
  CHECK(telemetry_build_json(in, small, sizeof(small)) == 0, "too-small buffer -> returns 0 (no partial packet)");
  CHECK(small[0] == '\0', "too-small buffer left empty");
  CHECK(telemetry_next_seq() == 1 && telemetry_next_seq() == 2, "sequence counter increments");
}

int main() {
  printf("CRASH CONNECT Week 2 - host unit tests (stubbed Arduino, real TinyGPS++)\n");
  test_state_machine();
  test_fault_flags();
  test_safety_and_imu();
  test_gnss();
  test_telemetry();
  printf("\nRESULT: %d passed, %d failed\n", passed, failed);
  return failed == 0 ? 0 : 1;
}
