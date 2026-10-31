#include <stdio.h>
#include <string.h>
#include "messages.h"
#include "../Common/json_out.h"
#include "../Config/config.h"
#include "../Device/device.h"
#include "../Modem/modem.h"

namespace {

uint32_t seqCounter = 0;
uint32_t eventCounter = 0;
uint32_t statusCounter = 0;
char dataSource[16] = "SIMULATED";

void header(JsonOut &j, const char *type, uint32_t now) {
  uint32_t epoch = 0;
  TimeSource src = TIME_NONE;
  bool have = clock_now(now, &epoch, &src);
  j.str("device_id", config().device_id);
  j.str("schema_version", SCHEMA_VERSION);
  j.str("message_type", type);
  j.u("sequence_number", true, ++seqCounter);
  j.u("timestamp_utc", have, epoch);
  j.str("time_source", time_source_name(src));
}

void fill_id(MsgItem *m, ItemKind k, const char *id) {
  memset(m, 0, sizeof(*m));
  m->kind = k;
  snprintf(m->id, sizeof(m->id), "%s", id);
}

}

void messages_boot_reset() {
  seqCounter = 0;
  eventCounter = 0;
  statusCounter = 0;
}

bool build_telemetry(const MsgContext &c, MsgItem *out) {
  char id[64];
  snprintf(id, sizeof(id), "T%lu", (unsigned long)(seqCounter + 1));
  fill_id(out, ITEM_TELEMETRY, id);
  JsonOut j(out->payload, sizeof(out->payload));
  const GnssState &g = *c.gnss;
  const GnssFix &f = g.current;
  const ImuSample &m = *c.imu;
  header(j, "telemetry", c.now_ms);
  j.str("data_source", dataSource);
  j.str("device_state", state_name(c.state));
  j.str("gnss_status", gnss_status_name(g.status));
  j.i("gnss_fix_quality", true, f.valid ? f.fix_quality : 0);
  j.num("latitude", f.valid, f.latitude, 6);
  j.num("longitude", f.valid, f.longitude, 6);
  j.num("speed_kmh", f.valid && f.speed_valid, f.speed_kmh, 1);
  j.num("course_deg", f.valid && f.course_valid, f.course_deg, 1);
  j.i("satellites", g.sats_valid, g.satellites);
  j.num("hdop", f.valid && f.hdop_valid, f.hdop, 2);
  j.u("last_fix_age_s", g.has_last_valid, gnss_last_valid_age_s(c.now_ms));
  j.num("accel_x_g", m.valid, m.ax, 3);
  j.num("accel_y_g", m.valid, m.ay, 3);
  j.num("accel_z_g", m.valid, m.az, 3);
  j.num("gyro_x_dps", m.valid, m.gx, 1);
  j.num("gyro_y_dps", m.valid, m.gy, 1);
  j.num("gyro_z_dps", m.valid, m.gz, 1);
  j.num("accel_magnitude_g", m.valid, m.mag_g, 3);
  j.u("battery_mv", false, 0);
  j.u("fault_flags", true, c.fault_flags);
  out->len = (uint16_t)j.finish();
  return out->len > 0;
}

bool build_status(const MsgContext &c, MsgItem *out) {
  char id[64];
  snprintf(id, sizeof(id), "%s-B%lu-S%lu", config().device_id, (unsigned long)config().boot_count,
           (unsigned long)++statusCounter);
  fill_id(out, ITEM_STATUS, id);
  JsonOut j(out->payload, sizeof(out->payload));
  header(j, "status", c.now_ms);
  int csq = 0;
  bool csqOk = modem_csq(&csq);
  j.str("firmware_version", FW_VERSION);
  j.u("uptime_s", true, c.now_ms / 1000u);
  j.str("reset_reason", reset_reason_name());
  j.u("boot_count", true, config().boot_count);
  j.flag("provisioned", config().provisioned);
  j.str("device_state", state_name(c.state));
  j.str("data_source", dataSource);
  j.flag("network_registered", modem_registered());
  j.i("signal_csq", csqOk, csq);
  j.i("signal_dbm", csqOk, -113 + 2 * csq);
  j.str("power_source", 0);
  j.u("battery_mv", false, 0);
  out->len = (uint16_t)j.finish();
  return out->len > 0;
}

bool build_event(EventKind kind, const DetectResult *det, const MsgContext &c, MsgItem *out) {
  char id[64];
  snprintf(id, sizeof(id), "%s-B%lu-E%lu", config().device_id, (unsigned long)config().boot_count,
           (unsigned long)++eventCounter);
  fill_id(out, ITEM_EVENT, id);
  JsonOut j(out->payload, sizeof(out->payload));
  const GnssState &g = *c.gnss;
  const GnssFix *loc = 0;
  const char *src = "NONE";
  if (g.current.valid) {
    loc = &g.current;
    src = "CURRENT";
  } else if (g.has_last_valid) {
    loc = &g.last_valid;
    src = "LAST_KNOWN";
  }
  uint32_t age = loc ? (c.now_ms - loc->fix_ms) / 1000u : 0;
  bool acc = kind == EVENT_ACCIDENT && det != 0;
  header(j, "event", c.now_ms);
  j.str("event_id", id);
  j.str("event_type", event_kind_name(kind));
  j.str("description", acc ? "Possible vehicle accident detected" : "SOS button pressed");
  j.num("latitude", loc != 0, loc ? loc->latitude : 0, 6);
  j.num("longitude", loc != 0, loc ? loc->longitude : 0, 6);
  j.i("gnss_fix_quality", true, g.current.valid ? g.current.fix_quality : 0);
  j.u("location_age_s", loc != 0, age);
  j.str("location_source", src);
  j.str("accident_type", acc ? det->accident_type : 0);
  j.str("confirmation_state", acc ? "CONFIRMED" : "USER_TRIGGERED");
  j.num("peak_accel_g", acc, acc ? det->peak_g : 0, 2);
  j.str("data_source", dataSource);
  j.str("snapshot", 0);
  out->len = (uint16_t)j.finish();
  return out->len > 0;
}

const char *motion_name(MotionState m) {
  switch (m) {
    case MOTION_MOVING:     return "MOVING";
    case MOTION_STATIONARY: return "STATIONARY";
    default:                return "UNKNOWN";
  }
}

MotionState motion_update(MotionState prev, const GnssState &g, const RingBuffer<ImuSample, 64> &rb) {
  float enter = (float)config().move_dkmh / 10.0f;
  float leave = enter * 0.4f;
  if (g.current.valid && g.current.speed_valid) {
    float v = g.current.speed_kmh;
    if (v >= enter) return MOTION_MOVING;
    if (v <= leave) return MOTION_STATIONARY;
    return prev == MOTION_UNKNOWN ? MOTION_STATIONARY : prev;
  }
  float sd = imu_motion_stddev(rb, 50);
  if (sd < 0) return prev;
  return sd >= 0.05f ? MOTION_MOVING : MOTION_STATIONARY;
}

uint32_t report_interval_ms(MotionState m, bool eventActive) {
  if (eventActive) return config().rpt_event_s * 1000u;
  if (m == MOTION_STATIONARY) return config().rpt_stationary_s * 1000u;
  return config().rpt_moving_s * 1000u;
}
