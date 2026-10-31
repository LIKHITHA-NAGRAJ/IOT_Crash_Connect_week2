#include <Arduino.h>
#include "src/common/cc_types.h"
#include "src/config/config.h"
#include "src/fault/fault.h"
#include "src/gnss/gnss.h"
#include "src/imu/imu.h"
#include "src/comms/comms.h"
#include "src/sos/sos.h"
#include "src/power/power.h"
#include "src/safety/safety_event.h"
#include "src/status/device_status.h"
#include "src/state/device_state.h"
#include "src/telemetry/telemetry.h"
#include "src/sim/sim_scenario.h"

static ImuSample g_imu;
static GnssFix g_fix;
static EventType g_event = EVENT_NONE;
static bool g_sos = false;
static bool g_imuOk = false;
static bool g_selfTestPass = false;
static bool g_doneReported = false;
static uint32_t g_doneAtMs = 0;
static uint32_t g_lastTick = 0;
static uint32_t g_lastImu = 0;
static uint32_t g_lastTelemetry = 0;

static void init_modules() {
  gnss_init();
  imu_init();
  comms_init();
  sos_init();
}

static bool run_self_test() {
  bool ok = true;
  if (!config_is_valid()) {
    fault_set(FAULT_CONFIG);
    ok = false;
  }
  if (!gnss_module_ok()) {
    fault_set(FAULT_GNSS);
    ok = false;
  }
  if (!imu_module_ok()) {
    fault_set(FAULT_IMU);
    ok = false;
  }
  if (!comms_module_ok()) {
    fault_set(FAULT_COMMS);
    ok = false;
  }
  if (!sos_module_ok()) {
    ok = false;
  }
  if (!ok) fault_set(FAULT_SELF_TEST);
  Serial.printf("[SELF_TEST] %s\n", ok ? "PASS" : "FAIL");
  return ok;
}

static void on_enter_state(DeviceState s) {
  if (s == STATE_BOOT) {
    init_modules();
  } else if (s == STATE_SELF_TEST) {
    g_selfTestPass = run_self_test();
  }
}

static void send_telemetry(uint32_t now, bool force) {
  DeviceState s = state_current();
  bool allowed = (s == STATE_OPERATIONAL || s == STATE_EVENT_ACTIVE || s == STATE_DEGRADED);
  if (!allowed) return;

  uint32_t interval = (s == STATE_EVENT_ACTIVE) ? config().timing.event_telemetry_ms
                                                : config().timing.telemetry_ms;
  if (!force && (now - g_lastTelemetry) < interval) return;
  g_lastTelemetry = now;

  TelemetryInput ti;
  ti.gnss = &g_fix;
  ti.imu = &g_imu;
  ti.state = s;
  ti.event = g_event;
  ti.sos_active = g_sos;
  ti.comms_connected = comms_is_connected();
  ti.battery_mv = 0;
  ti.battery_valid = power_read_battery_mv(&ti.battery_mv);
  ti.fault_flags = fault_flags();
  ti.uptime_ms = now;
  ti.seq = telemetry_next_seq();

  char buf[640];
  size_t n = telemetry_build_json(ti, buf, sizeof(buf));
  if (n == 0) {
    Serial.println("[TELEMETRY] ERROR: packet did not fit in buffer, not sent");
    return;
  }
  Serial.print("[TELEMETRY] ");
  Serial.println(buf);
  comms_send(config().comms.topic_telemetry, buf, n);
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("CRASH CONNECT - IoT firmware skeleton (Week 2)");
  Serial.printf("device=%s fw=%s packet=%s\n", config().device_id, config().fw_version, config().packet_version);
  Serial.println("Hardware access is STUBBED. GPS = recorded NMEA, IMU/SOS/network = test inputs.");

  if (!config_is_valid()) fault_set(FAULT_CONFIG);

  uint32_t now = millis();
  state_init(now);
  on_enter_state(STATE_BOOT);
  sim_init(now);
  g_lastImu = now;
  g_lastTelemetry = now;
  g_lastTick = now;
  status_get().state = STATE_BOOT;
  status_get().state_since_ms = now;
  Serial.printf("[STATE] -> %s\n", state_name(STATE_BOOT));
}

void loop() {
  uint32_t now = millis();
  if ((now - g_lastTick) < config().timing.tick_ms) return;
  g_lastTick = now;

  sim_update(now);
  gnss_poll_uart();
  comms_update(now);

  if ((now - g_lastImu) >= config().timing.imu_sample_ms) {
    g_lastImu = now;
    g_imuOk = imu_read(&g_imu);
    if (g_imuOk) fault_clear(FAULT_IMU);
    else fault_set(FAULT_IMU);
  }

  g_sos = sos_is_pressed();
  g_event = safety_evaluate(g_imu, g_sos);
  gnss_get_fix(&g_fix, now);

  DeviceStatus &st = status_get();
  st.gnss_module_ok = gnss_module_ok();
  st.gnss_fix = g_fix.valid;
  st.imu_ok = g_imuOk;
  st.comms_connected = comms_is_connected();
  st.sos_active = g_sos;
  st.event = g_event;
  st.fault_flags = fault_flags();

  StateInputs in;
  in.self_test_pass = g_selfTestPass;
  in.gps_fix = g_fix.valid;
  in.network_up = comms_is_connected();
  in.noncritical_fault = fault_is_noncritical();
  in.critical_fault = fault_is_critical();
  in.event = g_event;
  in.time_in_state_ms = 0;

  DeviceState previous = state_current();
  const char *reason = 0;
  bool changed = state_update(in, now, &previous, &reason);
  st.state = state_current();
  st.state_since_ms = state_entered_ms();

  if (changed) {
    Serial.printf("[STATE] t=%.1fs %s -> %s (%s)\n", now / 1000.0,
                  state_name(previous), state_name(state_current()), reason ? reason : "-");
    on_enter_state(state_current());
    status_print_line(now);
    if (state_current() == STATE_EVENT_ACTIVE) send_telemetry(now, true);
  }

  send_telemetry(now, false);

  if (sim_is_done() && !g_doneReported) {
    g_doneReported = true;
    g_doneAtMs = now;
    Serial.printf("[SIM] scenario complete. packets sent=%lu, not sent (network down)=%lu, checksum failures rejected=%lu\n",
                  (unsigned long)comms_sent_count(), (unsigned long)comms_failed_count(),
                  (unsigned long)gnss_checksum_failures());
    status_print_line(now);
  }
}
