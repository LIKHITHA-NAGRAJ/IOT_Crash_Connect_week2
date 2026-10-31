#include <Arduino.h>
#include <string.h>
#include "app.h"
#include "../Common/cc_types.h"
#include "../Common/log.h"
#include "../Config/config.h"
#include "../Device/device.h"
#include "../GNSS/gnss.h"
#include "../IMU/imu.h"
#include "../Accident_Detection/detector.h"
#include "../SOS_Events/sos_button.h"
#include "../SOS_Events/tx_manager.h"
#include "../Telemetry/messages.h"
#include "../State_Machine/device_state.h"

namespace {

const uint32_t FAULT_IMU = 1u << 0;
const uint32_t FAULT_SELF_TEST = 1u << 1;

AtPort *modemPort = 0;
AppStats stats;
RingBuffer<ImuSample, 64> ring;
ImuFilter filter;
SosButton button;
ImuSample lastImu;
MotionState motion = MOTION_UNKNOWN;
uint32_t lastTick = 0;
uint32_t lastTelemetry = 0;
uint32_t lastInterval = 0;
uint32_t lastClockSync = 0;
uint32_t eventUntil = 0;
uint32_t faultFlags = 0;
uint32_t imuFailRun = 0;
bool selfTestPass = false;
bool hung = false;
bool sosRaw = false;
bool wasReady = false;
EventKind activeEvent = EVENT_NONE;
GnssStatus lastGnssStatus = GNSS_NO_DATA;

MsgContext context(uint32_t now) {
  MsgContext c;
  c.now_ms = now;
  c.state = state_current();
  c.gnss = &gnss_state();
  c.imu = &lastImu;
  c.fault_flags = faultFlags;
  return c;
}

void raise_event(EventKind kind, const DetectResult *det, uint32_t now) {
  MsgItem item;
  MsgContext c = context(now);
  if (!build_event(kind, det, c, &item)) {
    LOGE("EVENT", ERR_PAYLOAD_TOO_LARGE, "event payload did not fit, not queued");
    return;
  }
  const GnssState &g = gnss_state();
  const char *src = g.current.valid ? "CURRENT" : (g.has_last_valid ? "LAST_KNOWN" : "NONE");
  LOGI("EVENT", "%s id=%s location=%s age=%lus", event_kind_name(kind), item.id, src,
       (unsigned long)(g.has_last_valid ? (now - (g.current.valid ? g.current.fix_ms : g.last_valid.fix_ms)) / 1000u : 0));
  LOGI("EVENT", "payload %s", item.payload);
  tx_enqueue(item);
  activeEvent = kind;
  eventUntil = now + timing().event_hold_ms;
  if (kind == EVENT_ACCIDENT) stats.accident_events++;
  else stats.sos_events++;
}

bool run_self_test() {
  bool ok = modemPort != 0;
  if (!ok) faultFlags |= FAULT_SELF_TEST;
  if (ok) LOGI("SELFTEST", "PASS"); else LOGE("SELFTEST", ERR_SELF_TEST_FAILED, "FAIL");
  return ok;
}

void on_enter(DeviceState s) {
  if (s == STATE_SELF_TEST) selfTestPass = run_self_test();
}

void send_telemetry(uint32_t now) {
  DeviceState s = state_current();
  if (s != STATE_OPERATIONAL && s != STATE_EVENT_ACTIVE && s != STATE_DEGRADED) return;
  motion = motion_update(motion, gnss_state(), ring);
  uint32_t interval = report_interval_ms(motion, activeEvent != EVENT_NONE);
  if (interval != lastInterval) {
    LOGI("RATE", "motion=%s event=%d interval=%lus", motion_name(motion), activeEvent != EVENT_NONE ? 1 : 0,
         (unsigned long)(interval / 1000u));
    lastInterval = interval;
  }
  if ((now - lastTelemetry) < interval) return;
  lastTelemetry = now;
  MsgItem item;
  if (!build_telemetry(context(now), &item)) {
    LOGE("TELEMETRY", ERR_PAYLOAD_TOO_LARGE, "packet did not fit, not queued");
    return;
  }
  stats.telemetry_built++;
  LOGI("TELEMETRY", "%s", item.payload);
  tx_enqueue(item);
}

}

void app_setup(uint32_t now, AtPort *port, bool simulatedClock) {
  modemPort = port;
  ConfigReport rep = config_load();
  config_bump_boot_count();
  char desc[300];
  config_describe(desc, sizeof(desc));
  LOGI("BOOT", "CRASH CONNECT firmware %s reset_reason=%s", FW_VERSION, reset_reason_name());
  LOGI("BOOT", "config loaded=%d defaulted=%d: %s", rep.loaded, rep.defaulted, desc);
  clock_reset();
  if (simulatedClock) clock_set(epoch_from_ymdhms(2026, 10, 31, 8, 0, 0), TIME_SIMULATED, now);
  messages_boot_reset();
  tx_init();
  detector_init(detector_default_config());
  imu_filter_init(&filter, 0.3f);
  sos_button_init(&button);
  ring.clear();
  memset(&lastImu, 0, sizeof(lastImu));
  motion = MOTION_UNKNOWN;
  lastTick = now;
  lastTelemetry = now;
  lastInterval = 0;
  eventUntil = 0;
  faultFlags = 0;
  imuFailRun = 0;
  activeEvent = EVENT_NONE;
  wasReady = false;
  hung = false;
  sosRaw = false;
  lastGnssStatus = GNSS_NO_DATA;
  gnss_init();
  imu_init();
  wd_init(config().wd_timeout_s, now);
  state_init(now);
  if (modemPort) modem_init(modemPort, now);
  LOGI("STATE", "-> BOOT");
}

void app_sim_set_sos(bool pressed) { sosRaw = pressed; }
void app_sim_hang(bool h) { hung = h; }
const AppStats &app_stats() { return stats; }

bool app_sim_watchdog_check(uint32_t now) {
  if (!wd_sim_expired(now)) return false;
  LOGE("WATCHDOG", ERR_WATCHDOG_RESET, "no feed for more than %lus, simulated hardware reset",
       (unsigned long)config().wd_timeout_s);
  reset_reason_sim_set("TASK_WDT");
  stats.resets++;
  return true;
}

void app_loop(uint32_t now) {
  if (hung) return;
  if ((now - lastTick) < timing().tick_ms) return;
  lastTick = now;
  wd_feed(now);

  gnss_update(now);
  const GnssState &g = gnss_state();
  if (g.status != lastGnssStatus) {
    if (g.status == GNSS_NO_FIX) LOGW("GNSS", ERR_GNSS_NO_FIX, "no fix, no coordinates will be reported");
    else if (g.status == GNSS_STALE) LOGW("GNSS", ERR_GNSS_STALE, "no NMEA data, last fix is %lus old", (unsigned long)gnss_last_valid_age_s(now));
    else if (g.status == GNSS_NO_DATA) LOGW("GNSS", ERR_GNSS_NO_DATA, "no NMEA data yet");
    else LOGI("GNSS", "fix acquired quality=%d satellites=%d", g.fix_quality, g.satellites);
    lastGnssStatus = g.status;
  }
  if (g.current.valid && g.current.utc_valid && (now - lastClockSync) > 60000u) {
    clock_set(g.current.utc_epoch, TIME_GNSS, now);
    lastClockSync = now;
  }

  ImuSample s;
  if (imu_read(&s)) {
    s.t_ms = now;
    lastImu = s;
    ring.push(s);
    imu_filter_step(&filter, s);
    imuFailRun = 0;
    faultFlags &= ~FAULT_IMU;
    bool sv = g.current.valid && g.current.speed_valid;
    DetectResult d = detector_step(s, sv, sv ? g.current.speed_kmh : 0);
    if (d.decision == DET_CANDIDATE) {
      LOGI("DETECT", "candidate: %s (peak %.2f g)", d.reason, s.mag_g);
    } else if (d.decision == DET_REJECTED) {
      LOGI("DETECT", "rejected: %s (peak %.2f g, speed_check=%s)", d.reason, d.peak_g, d.speed_check);
    } else if (d.decision == DET_CONFIRMED) {
      LOGI("DETECT", "confirmed: %s peak=%.2f g rotation=%.0f deg speed_check=%s after %lums", d.accident_type,
           d.peak_g, d.max_rotation_deg, d.speed_check, (unsigned long)d.decision_after_ms);
      raise_event(EVENT_ACCIDENT, &d, now);
    }
  } else {
    stats.imu_invalid++;
    if (++imuFailRun == 25) LOGW("IMU", ERR_IMU_SAMPLE_INVALID, "25 consecutive invalid samples");
    if (imuFailRun >= 25) faultFlags |= FAULT_IMU;
  }

  uint32_t held = 0;
  SosAction a = sos_button_step(&button, sosRaw, now, config().sos_debounce_ms, config().sos_hold_ms, &held);
  if (a == SOS_PRESS_BEGIN) LOGI("SOS", "press detected, keep holding %lums", (unsigned long)config().sos_hold_ms);
  else if (a == SOS_RELEASED_EARLY) LOGI("SOS", "released after %lums, cancelled (short press ignored)", (unsigned long)held);
  else if (a == SOS_TRIGGERED) {
    LOGI("SOS", "long press confirmed after %lums, SOS activated", (unsigned long)held);
    raise_event(EVENT_SOS, 0, now);
  }

  modem_update(now);
  bool ready = modem_ready() || modem_state() == MODEM_POSTING;
  if (ready && !wasReady) {
    MsgItem item;
    if (build_status(context(now), &item)) {
      stats.status_built++;
      LOGI("STATUS", "connected, queueing status message id=%s", item.id);
      LOGI("STATUS", "payload %s", item.payload);
      tx_enqueue(item);
    }
  }
  wasReady = ready;
  tx_update(now);

  if (activeEvent != EVENT_NONE && now >= eventUntil) activeEvent = EVENT_NONE;

  StateInputs in;
  in.self_test_pass = selfTestPass;
  in.gps_fix = g.current.valid;
  in.network_up = modem_ready() || modem_state() == MODEM_POSTING;
  in.noncritical_fault = (faultFlags & FAULT_IMU) != 0;
  in.critical_fault = false;
  in.event = activeEvent;
  in.time_in_state_ms = 0;
  DeviceState prev = state_current();
  const char *reason = 0;
  if (state_update(in, now, &prev, &reason)) {
    LOGI("STATE", "%s -> %s (%s)", state_name(prev), state_name(state_current()), reason ? reason : "-");
    on_enter(state_current());
  }
  send_telemetry(now);
}
