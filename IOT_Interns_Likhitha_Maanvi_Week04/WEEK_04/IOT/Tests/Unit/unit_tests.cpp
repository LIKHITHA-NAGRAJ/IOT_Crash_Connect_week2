#include <Arduino.h>
#include <math.h>
#include <string.h>
#include <stdio.h>
#include "Common/errors.h"
#include "Common/log.h"
#include "Config/config.h"
#include "Device/device.h"
#include "GNSS/gnss.h"
#include "IMU/imu.h"
#include "Accident_Detection/detector.h"
#include "SOS_Events/sos_button.h"
#include "SOS_Events/msg_queue.h"
#include "SOS_Events/tx_manager.h"
#include "Telemetry/messages.h"
#include "State_Machine/device_state.h"
#include "Sim/mock_at_port.h"
#include "Sim/nmea_gen.h"
#include "Storage/kv_store.h"

unsigned long fake_ms = 0;
SerialClass Serial;

static int passed = 0, failed = 0;

#define CHECK(cond, name) do { if (cond) { passed++; printf("PASS  %s\n", name); } else { failed++; printf("FAIL  %s  (%s:%d)\n", name, __FILE__, __LINE__); } } while (0)

static void run_modem(MockAtPort &p, uint32_t from, uint32_t to) {
  for (uint32_t t = from; t <= to; t += 20) {
    fake_ms = t;
    modem_update(t);
  }
  (void)p;
}

static void test_errors_and_log() {
  printf("\n== Error table and logging ==\n");
  bool unique = true;
  for (int i = 0; i < err_table_size(); ++i)
    for (int j = i + 1; j < err_table_size(); ++j) {
      ErrCode a, b; const char *n; ErrSeverity s; const char *t;
      err_table_at(i, &a, &n, &s, &t);
      err_table_at(j, &b, &n, &s, &t);
      if (a == b) unique = false;
    }
  CHECK(unique, "error codes are unique");
  CHECK(strcmp(err_name(ERR_TX_NO_SERVER), "ERR_TX_NO_SERVER") == 0, "error name lookup");
  CHECK(err_severity(ERR_WATCHDOG_RESET) == SEV_CRITICAL, "severity lookup");
  log_reset_counters();
  log_set_level(CCLOG_ERROR);
  LOGI("T", "hidden");
  LOGW("T", ERR_GNSS_NO_FIX, "counted but hidden");
  CHECK(log_count(CCLOG_INFO) == 1 && log_count(CCLOG_WARN) == 1, "levels are counted even when filtered");
  CHECK(log_last_error() == ERR_GNSS_NO_FIX, "last error remembered");
  log_set_level(CCLOG_INFO);
}

static void test_storage_config() {
  printf("\n== Configuration storage (HOST SIMULATION of NVS) ==\n");
  kv_erase_all();
  kv_begin("crashconn");
  ConfigReport r = config_load();
  CHECK(!config().provisioned && strcmp(config().device_id, "UNPROVISIONED") == 0, "missing device_id reported as UNPROVISIONED");
  CHECK(r.loaded == 0, "nothing loaded from empty storage");
  CHECK(config().rpt_moving_s == 5 && config().rpt_stationary_s == 30 && config().rpt_event_s == 2, "default intervals 5/30/2 s");
  CHECK(config_provision("DEVICE-001"), "provision device id");
  CHECK(config_set_u32("rpt_still_s", 45), "set stationary interval");
  CHECK(!config_set_u32("rpt_still_s", 0), "reject interval 0");
  CHECK(!config_set_u32("rpt_move_s", 100), "reject moving interval larger than stationary");
  CHECK(!config_set_u32("no_such_key", 1), "reject unknown key");
  config_load();
  CHECK(config().provisioned && strcmp(config().device_id, "DEVICE-001") == 0, "device_id survives reload");
  CHECK(config().rpt_stationary_s == 45, "interval survives reload");
  kv_put_u32("q_tel_cap", 9999);
  config_load();
  CHECK(config().q_tel_cap == 50, "out-of-range stored value falls back to default");
  uint32_t b1 = config_bump_boot_count();
  config_load();
  CHECK(config().boot_count == b1, "boot count persisted");
  config_set_u32("rpt_still_s", 30);
}

static void test_clock() {
  printf("\n== Device clock ==\n");
  clock_reset();
  uint32_t e; TimeSource s;
  CHECK(!clock_now(0, &e, &s) && s == TIME_NONE, "no time source before any time is set");
  CHECK(epoch_from_ymdhms(1970, 1, 1, 0, 0, 0) == 0, "epoch of 1970-01-01");
  CHECK(epoch_from_ymdhms(2026, 10, 31, 8, 0, 0) == 1793433600u, "epoch of 2026-10-31 08:00:00");
  clock_set(epoch_from_ymdhms(1994, 3, 23, 12, 35, 19), TIME_GNSS, 0);
  CHECK(!clock_now(0, &e, &s), "recorded 1994 GNSS date refused as live time");
  clock_set(1793433600u, TIME_SIMULATED, 1000);
  CHECK(clock_now(6000, &e, &s) && e == 1793433605u && s == TIME_SIMULATED, "clock advances from uptime");
}

static void feed_recorded(const char *text) { gnss_feed(text); }

static void test_gnss() {
  printf("\n== GNSS handling (RECORDED NMEA samples from Week 1/2 sensor_data) ==\n");
  gnss_init();
  fake_ms = 1000;
  gnss_update(1000);
  CHECK(gnss_state().status == GNSS_NO_DATA, "no data before any sentence");
  feed_recorded("$GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47\r\n"
                "$GPRMC,123519,A,4807.038,N,01131.000,E,022.4,084.4,230394,003.1,W*6A\r\n");
  gnss_update(1000);
  const GnssState &g = gnss_state();
  CHECK(g.status == GNSS_FIX && g.current.valid, "valid fix from GP sentences");
  CHECK(fabs(g.current.latitude - 48.1173) < 0.0005 && fabs(g.current.longitude - 11.5167) < 0.0005, "coordinates decoded");
  CHECK(g.current.speed_valid && fabs(g.current.speed_kmh - 41.5) < 0.2, "speed decoded from knots to km/h");
  CHECK(!g.current.utc_valid, "recorded 1994 date is not used as UTC");
  fake_ms = 3000;
  feed_recorded("$GPGGA,123521,,,,,0,00,99.99,,,,,,*4E\r\n$GPRMC,123521,V,,,,,,,230394,,*38\r\n");
  gnss_update(3000);
  CHECK(gnss_state().status == GNSS_NO_FIX && !gnss_state().current.valid, "NO_FIX after no-fix sentence");
  CHECK(gnss_state().has_last_valid, "last valid fix is remembered");
  CHECK(gnss_last_valid_age_s(3000) == 2, "last valid fix age is 2 s");
  fake_ms = 20000;
  gnss_update(20000);
  CHECK(gnss_state().status == GNSS_STALE, "STALE when receiver stops sending");
  uint32_t before = gnss_checksum_failures();
  fake_ms = 21000;
  feed_recorded("$GPGGA,123522,4807.050,N,01131.020,E,1,08,0.9,545.7,M,46.9,M,,*00\r\n");
  CHECK(gnss_checksum_failures() == before + 1, "wrong checksum rejected");
  gnss_init();
  fake_ms = 30000;
  feed_recorded("$GNGGA,123519,4807.038,N,01131.000,E,1,10,0.8,545.4,M,46.9,M,,*51\r\n"
                "$GNRMC,123519,A,4807.038,N,01131.000,E,022.4,084.4,230394,003.1,W,A*19\r\n");
  gnss_update(30000);
  CHECK(gnss_state().status == GNSS_FIX, "GN talker accepted");
  char buf[400];
  gnss_init();
  fake_ms = 40000;
  nmea_pair(8 * 3600, true, 10.0, buf, sizeof(buf));
  gnss_feed(buf);
  gnss_update(40000);
  CHECK(gnss_state().current.utc_valid && gnss_state().current.utc_epoch == 1793433600u, "generated 2026 date gives UTC epoch");
}

static void test_imu() {
  printf("\n== IMU, ring buffer, filter, CSV ==\n");
  RingBuffer<ImuSample, 64> rb;
  for (int i = 0; i < 70; ++i) {
    ImuSample s; memset(&s, 0, sizeof(s));
    s.valid = true; s.t_ms = (uint32_t)i; s.mag_g = 1.0f;
    rb.push(s);
  }
  CHECK(rb.size() == 64 && rb.overwritten() == 6, "ring buffer is bounded and overwrites oldest");
  CHECK(rb.at(0).t_ms == 6 && rb.at(63).t_ms == 69, "ring buffer keeps order oldest to newest");
  CHECK(fabsf(imu_motion_stddev(rb, 50)) < 0.001f, "constant signal has zero motion deviation");
  ImuFilter f; imu_filter_init(&f, 0.5f);
  ImuSample s; memset(&s, 0, sizeof(s)); s.valid = true; s.mag_g = 1.0f;
  imu_filter_step(&f, s);
  s.mag_g = 3.0f;
  CHECK(fabsf(imu_filter_step(&f, s) - 2.0f) < 0.001f, "low-pass filter step");
  ReplayRow r;
  CHECK(imu_csv_parse("1000,0.02,0.01,0.98,0.5,0.2,1.0,41.5,1", &r) && r.speed_valid && fabsf(r.imu.mag_g - 0.98f) < 0.01f, "CSV row parsed");
  CHECK(!imu_csv_parse("t_ms,ax_g", &r), "CSV header skipped");
  CHECK(!imu_csv_parse("1000,nan,nan,nan,1,1,1,30,1", &r), "CSV row with nan rejected");
  CHECK(imu_csv_parse("1000,0.02,0.01,0.98,0.5,0.2,1.0,,0", &r) && !r.speed_valid, "no GPS fix gives no speed");
  char out[160];
  imu_csv_format(r, out, sizeof(out));
  ReplayRow r2;
  CHECK(imu_csv_parse(out, &r2) && fabsf(r2.imu.ax - r.imu.ax) < 0.001f, "CSV format and parse round trip");
}

static ImuSample mk(uint32_t t, float ax, float ay, float az, float g) {
  ImuSample s; memset(&s, 0, sizeof(s));
  s.t_ms = t; s.valid = true; s.ax = ax; s.ay = ay; s.az = az; s.gx = g; s.gy = 0; s.gz = 0;
  s.mag_g = imu_magnitude(ax, ay, az);
  return s;
}

static void test_detector() {
  printf("\n== Accident detector stages (SIMULATED samples) ==\n");
  detector_init(detector_default_config());
  uint32_t t = 0;
  for (; t < 1000; t += 20) detector_step(mk(t, 0, 0, 1, 0), true, 50);
  DetectResult d = detector_step(mk(t, 0, 0, 5.0f, 0), true, 50); t += 20;
  CHECK(d.decision == DET_NONE, "single spike alone does not trigger");
  d = detector_step(mk(t, 0, 0, 1, 0), true, 50); t += 20;
  CHECK(detector_counters().triggers == 0, "one-sample spike (pothole) ignored");
  d = detector_step(mk(t, 5, 0, 1, 0), true, 50); t += 20;
  d = detector_step(mk(t, 5, 0, 1, 0), true, 50); t += 20;
  CHECK(d.decision == DET_CANDIDATE, "two samples above trigger give a candidate");
  bool confirmed = false;
  for (int i = 0; i < 400 && !confirmed; ++i, t += 20) {
    d = detector_step(mk(t, 0, 0, 1, 0), true, i < 60 ? 30 : 0);
    if (d.decision == DET_CONFIRMED) confirmed = true;
  }
  CHECK(confirmed && strcmp(d.accident_type, "IMPACT") == 0, "stillness plus stopped GPS confirms IMPACT");
  CHECK(detector_counters().confirmed == 1, "confirmed counter");
  detector_init(detector_default_config());
  ImuSample bad; memset(&bad, 0, sizeof(bad));
  detector_step(bad, false, 0);
  CHECK(detector_counters().invalid_samples == 1, "invalid sample counted and ignored");
}

static void test_sos() {
  printf("\n== SOS long press and debounce (SIMULATED button) ==\n");
  SosButton b; sos_button_init(&b);
  uint32_t held = 0; SosAction a = SOS_NONE;
  int triggers = 0, early = 0;
  for (uint32_t t = 0; t < 100; t += 5) {
    a = sos_button_step(&b, (t / 5) % 2 == 0, t, 50, 2000, &held);
    if (a == SOS_TRIGGERED) triggers++;
  }
  CHECK(triggers == 0, "contact bounce shorter than debounce is ignored");
  sos_button_init(&b);
  int begins = 0;
  uint32_t earlyHeld = 0;
  for (uint32_t t = 0; t < 3000; t += 10) {
    a = sos_button_step(&b, t >= 100 && t < 1100, t, 50, 2000, &held);
    if (a == SOS_PRESS_BEGIN) begins++;
    if (a == SOS_RELEASED_EARLY) { early++; earlyHeld = held; }
    if (a == SOS_TRIGGERED) triggers++;
  }
  CHECK(begins == 1 && early == 1 && triggers == 0 && earlyHeld >= 900, "1 s press is cancelled");
  sos_button_init(&b);
  triggers = 0;
  uint32_t trigAt = 0;
  for (uint32_t t = 0; t < 6000; t += 10) {
    a = sos_button_step(&b, t >= 100 && t < 5000, t, 50, 2000, &held);
    if (a == SOS_TRIGGERED) { triggers++; trigAt = t; }
  }
  CHECK(triggers == 1 && trigAt >= 2100 && trigAt <= 2200, "long press triggers once after the hold time");
}

static void test_queue() {
  printf("\n== Message queue (RAM model of the flash-backed queue) ==\n");
  MsgQueue q; q.init(3, OVERFLOW_DROP_NEWEST);
  MsgItem it, dropped;
  for (int i = 1; i <= 4; ++i) {
    memset(&it, 0, sizeof(it));
    snprintf(it.id, sizeof(it.id), "evt-%d", i);
    PushResult r = q.push(it, &dropped);
    if (i == 4) CHECK(r == PUSH_DROPPED_NEWEST && strcmp(dropped.id, "evt-4") == 0, "drop-newest policy rejects the 4th record");
  }
  CHECK(q.size() == 3 && strcmp(q.head()->id, "evt-1") == 0, "FIFO head is the oldest record");
  q.pop();
  CHECK(strcmp(q.head()->id, "evt-2") == 0, "FIFO order after pop");
  MsgQueue o; o.init(2, OVERFLOW_DROP_OLDEST);
  for (int i = 1; i <= 3; ++i) { snprintf(it.id, sizeof(it.id), "e%d", i); o.push(it, &dropped); }
  CHECK(strcmp(o.head()->id, "e2") == 0 && strcmp(dropped.id, "e1") == 0 && o.dropped_oldest() == 1, "drop-oldest policy evicts the oldest");
}

static void test_modem() {
  printf("\n== Modem state machine with MOCK AT responses ==\n");
  config_load();
  MockAtPort p;
  modem_init(&p, 0);
  run_modem(p, 0, 20000);
  CHECK(modem_ready() && p.power_on_count == 1, "bring-up reaches READY (mock)");
  int csq = 0;
  CHECK(modem_csq(&csq) && csq == 18, "signal quality parsed from AT+CSQ (mock)");
  p.registered = false;
  run_modem(p, 20020, 26000);
  CHECK(!modem_ready() && modem_state() == MODEM_REGISTERING, "registration loss detected");
  p.registered = true;
  run_modem(p, 26020, 36000);
  CHECK(modem_ready(), "recovers when registration returns");

  MockAtPort p2; p2.sim_ready = false;
  modem_init(&p2, 0);
  run_modem(p2, 0, 20000);
  CHECK(!modem_ready() && modem_last_error() == ERR_MODEM_SIM_NOT_READY, "SIM not ready is reported");

  MockAtPort p3; p3.responds = false;
  modem_init(&p3, 0);
  run_modem(p3, 0, 30000);
  CHECK(modem_last_error() == ERR_MODEM_NO_RESPONSE && modem_failures() >= 1, "no AT answer is reported");

  MockAtPort p4; p4.registered = false;
  modem_init(&p4, 0);
  run_modem(p4, 0, 60000);
  CHECK(modem_last_error() == ERR_MODEM_NOT_REGISTERED, "registration timeout is reported");

  MockAtPort p5;
  modem_init(&p5, 0);
  run_modem(p5, 0, 20000);
  CHECK(modem_post_begin(20000, "{\"a\":1}", 7), "post accepted when READY");
  run_modem(p5, 20020, 24000);
  int status = 0;
  CHECK(modem_post_poll(&status) == POST_ACKED && status == 200 && p5.received_count == 1, "HTTP 200 from mock server is an ACK");
}

static void pump(MockAtPort &p, uint32_t from, uint32_t to) {
  (void)p;
  for (uint32_t t = from; t <= to; t += 20) {
    fake_ms = t;
    modem_update(t);
    tx_update(t);
  }
}

static MsgItem mkmsg(const char *id, ItemKind k) {
  MsgItem m; memset(&m, 0, sizeof(m));
  snprintf(m.id, sizeof(m.id), "%s", id);
  m.kind = k;
  m.len = (uint16_t)snprintf(m.payload, sizeof(m.payload), "{\"event_id\":\"%s\"}", id);
  return m;
}

static void test_tx() {
  printf("\n== Retry, acknowledgement and offline queue (MOCK modem and server) ==\n");
  config_load();
  MockAtPort p;
  modem_init(&p, 0);
  tx_init();
  p.registered = false;
  pump(p, 0, 20000);
  tx_enqueue(mkmsg("A-1", ITEM_EVENT));
  tx_enqueue(mkmsg("A-2", ITEM_EVENT));
  pump(p, 20020, 30000);
  CHECK(tx_stats().acked == 0 && tx_events().size() == 2, "offline: events stay queued, nothing sent");
  p.registered = true;
  pump(p, 30020, 70000);
  CHECK(tx_events().size() == 0 && tx_stats().acked == 2, "online: queue emptied after ACKs");
  CHECK(strcmp(p.received_ids[0], "A-1") == 0 && strcmp(p.received_ids[1], "A-2") == 0, "delivered in FIFO order");

  MockAtPort q;
  modem_init(&q, 0);
  tx_init();
  pump(q, 0, 20000);
  q.server = SERVER_DOWN;
  tx_enqueue(mkmsg("B-1", ITEM_EVENT));
  pump(q, 20020, 60000);
  CHECK(tx_stats().acked == 0 && tx_stats().retries >= 2 && tx_events().size() == 1, "no server: event retained and retried");
  q.server = SERVER_OK;
  pump(q, 60020, 100000);
  CHECK(tx_stats().acked == 1 && tx_events().size() == 0, "delivered after server returns");
  bool same = q.received_count >= 1;
  for (uint32_t i = 0; i < q.received_count; ++i) if (strcmp(q.received_ids[i], "B-1") != 0) same = false;
  CHECK(same, "same event id on every attempt");
  CHECK(tx_events().head() == 0, "queue empty at end");

  MockAtPort r;
  modem_init(&r, 0);
  tx_init();
  pump(r, 0, 20000);
  r.server = SERVER_REJECT_400;
  tx_enqueue(mkmsg("C-1", ITEM_EVENT));
  pump(r, 20020, 30000);
  CHECK(tx_stats().rejected_dropped == 1 && tx_events().size() == 0, "HTTP 400 drops the record with an error code");

  MockAtPort s;
  modem_init(&s, 0);
  tx_init();
  s.server = SERVER_ERROR_503;
  pump(s, 0, 20000);
  tx_enqueue(mkmsg("D-1", ITEM_EVENT));
  pump(s, 20020, 40000);
  CHECK(tx_stats().retries >= 1 && tx_backoff_s() >= 1, "HTTP 503 is retried");
  uint32_t b1 = tx_backoff_s();
  pump(s, 40020, 90000);
  CHECK(tx_backoff_s() <= 30 && tx_backoff_s() >= b1, "backoff grows and stays at or below 30 s");

  MockAtPort u;
  modem_init(&u, 0);
  tx_init();
  pump(u, 0, 20000);
  tx_enqueue(mkmsg("T-1", ITEM_TELEMETRY));
  tx_enqueue(mkmsg("E-1", ITEM_EVENT));
  pump(u, 20020, 30000);
  CHECK(strcmp(u.received_ids[0], "E-1") == 0, "events lane is sent before telemetry lane");
}

static void test_messages() {
  printf("\n== Flat message builders ==\n");
  config_load();
  config_provision("DEVICE-001");
  messages_boot_reset();
  clock_reset();
  MockAtPort statusPort;
  modem_init(&statusPort, 0);
  GnssState g; memset(&g, 0, sizeof(g));
  ImuSample m; memset(&m, 0, sizeof(m));
  MsgContext c; c.now_ms = 5000; c.state = STATE_DEGRADED; c.gnss = &g; c.imu = &m; c.fault_flags = 0;
  MsgItem it;
  CHECK(build_telemetry(c, &it) && strstr(it.payload, "\"latitude\":null") && strstr(it.payload, "\"gnss_fix_quality\":0"), "no fix: null coordinates, fix quality 0");
  CHECK(strstr(it.payload, "\"timestamp_utc\":null") && strstr(it.payload, "\"time_source\":\"NONE\""), "no time source: null timestamp");
  CHECK(strstr(it.payload, "\"battery_mv\":null") && strstr(it.payload, "\"speed_kmh\":null"), "unavailable values are null");
  CHECK(strchr(it.payload + 1, '{') == 0, "telemetry is flat (no nested object)");
  CHECK(build_status(c, &it) && strstr(it.payload, "\"signal_csq\":null") && strstr(it.payload, "\"power_source\":null"), "status: unavailable signal and power are null");
  CHECK(strstr(it.payload, "\"firmware_version\":\"0.4.0\"") && strstr(it.payload, "\"reset_reason\"") && strstr(it.payload, "\"uptime_s\":5"), "status: firmware version, reset reason, uptime");
  CHECK(build_event(EVENT_SOS, 0, c, &it) && strstr(it.payload, "\"location_source\":\"NONE\"") && strstr(it.payload, "\"latitude\":null"), "SOS with no GPS ever: no coordinates");
  CHECK(strstr(it.payload, "\"event_id\":\"DEVICE-001-B") && strstr(it.payload, "\"confirmation_state\":\"USER_TRIGGERED\""), "SOS event id and state");
  MsgItem it2;
  build_event(EVENT_SOS, 0, c, &it2);
  CHECK(strcmp(it.id, it2.id) != 0, "event ids are unique");
  g.has_last_valid = true; g.last_valid.valid = true; g.last_valid.latitude = 48.1; g.last_valid.longitude = 11.5; g.last_valid.fix_ms = 1000; g.last_valid.fix_quality = 1;
  c.now_ms = 9000;
  DetectResult d; memset(&d, 0, sizeof(d)); snprintf(d.accident_type, sizeof(d.accident_type), "IMPACT"); d.peak_g = 8.1f;
  CHECK(build_event(EVENT_ACCIDENT, &d, c, &it) && strstr(it.payload, "\"location_source\":\"LAST_KNOWN\"") && strstr(it.payload, "\"location_age_s\":8"), "accident with last known location shows its age");
  CHECK(strstr(it.payload, "\"accident_type\":\"IMPACT\"") && strstr(it.payload, "\"confirmation_state\":\"CONFIRMED\""), "accident type and confirmation state");
}

static void test_rate() {
  printf("\n== Adaptive reporting rate ==\n");
  config_load();
  RingBuffer<ImuSample, 64> rb;
  GnssState g; memset(&g, 0, sizeof(g));
  g.current.valid = true; g.current.speed_valid = true;
  g.current.speed_kmh = 30;
  MotionState m = motion_update(MOTION_UNKNOWN, g, rb);
  CHECK(m == MOTION_MOVING && report_interval_ms(m, false) == 5000, "moving: 5 s");
  g.current.speed_kmh = 1;
  m = motion_update(m, g, rb);
  CHECK(m == MOTION_STATIONARY && report_interval_ms(m, false) == 30000, "stationary: 30 s");
  g.current.speed_kmh = 3;
  CHECK(motion_update(MOTION_STATIONARY, g, rb) == MOTION_STATIONARY, "hysteresis keeps stationary between 2 and 5 km/h");
  CHECK(report_interval_ms(m, true) == 2000, "event active: 2 s");
  config_set_u32("rpt_still_s", 12);
  CHECK(report_interval_ms(MOTION_STATIONARY, false) == 12000, "changing stored interval changes behaviour");
  config_set_u32("rpt_still_s", 30);
  g.current.valid = false;
  for (int i = 0; i < 40; ++i) { ImuSample s; memset(&s, 0, sizeof(s)); s.valid = true; s.mag_g = 1.0f + ((i % 2) ? 0.2f : -0.2f); rb.push(s); }
  CHECK(motion_update(MOTION_UNKNOWN, g, rb) == MOTION_MOVING, "no GPS: IMU vibration used to decide moving");
}

static void test_state() {
  printf("\n== State machine ==\n");
  config_load();
  StateInputs in; memset(&in, 0, sizeof(in));
  const char *why = 0;
  in.event = EVENT_SOS;
  CHECK(state_next(STATE_ACQUIRING_GPS, in, &why) == STATE_EVENT_ACTIVE, "SOS accepted while acquiring GPS");
  CHECK(state_next(STATE_NETWORK_CONNECTING, in, &why) == STATE_EVENT_ACTIVE, "SOS accepted while connecting");
  CHECK(state_next(STATE_DEGRADED, in, &why) == STATE_EVENT_ACTIVE, "SOS accepted while degraded");
  CHECK(state_next(STATE_BOOT, in, &why) == STATE_BOOT, "event ignored during BOOT");
  in.event = EVENT_NONE; in.critical_fault = true;
  bool all = true;
  for (int s = 0; s < STATE_COUNT; ++s) if (s != STATE_FAULT && state_next((DeviceState)s, in, &why) != STATE_FAULT) all = false;
  CHECK(all, "critical fault from every state goes to FAULT");
  memset(&in, 0, sizeof(in));
  in.gps_fix = true; in.network_up = true;
  CHECK(state_next(STATE_DEGRADED, in, &why) == STATE_OPERATIONAL, "recovery to OPERATIONAL");
  in.gps_fix = false;
  CHECK(state_next(STATE_OPERATIONAL, in, &why) == STATE_DEGRADED, "GPS loss degrades");
}

static void test_watchdog() {
  printf("\n== Watchdog (SIMULATED forced hang) ==\n");
  wd_init(8, 0);
  wd_feed(1000);
  CHECK(!wd_sim_expired(8000), "fed watchdog does not expire");
  CHECK(wd_sim_expired(9100), "unfed watchdog expires after the timeout");
  reset_reason_sim_set("TASK_WDT");
  CHECK(strcmp(reset_reason_name(), "TASK_WDT") == 0, "reset reason reported after watchdog reset");
}

int main() {
  setvbuf(stdout, 0, _IOLBF, 0);
  printf("CRASH CONNECT v0.4 unit tests - HOST PC, SIMULATED/MOCK inputs, not run on ESP32-S3\n");
  kv_begin("crashconn");
  test_errors_and_log();
  test_storage_config();
  test_clock();
  test_gnss();
  test_imu();
  test_detector();
  test_sos();
  test_queue();
  test_modem();
  test_tx();
  test_messages();
  test_rate();
  test_state();
  test_watchdog();
  printf("\nTOTAL: %d passed, %d failed\n", passed, failed);
  return failed ? 1 : 0;
}
