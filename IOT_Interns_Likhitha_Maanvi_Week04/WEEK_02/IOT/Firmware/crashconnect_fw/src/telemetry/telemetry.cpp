#include "telemetry.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "../config/config.h"

namespace {

struct Out {
  char *buf;
  size_t cap;
  size_t len;
  bool ok;
};

uint32_t seqCounter = 0;

void put(Out &o, const char *fmt, ...) {
  if (!o.ok) return;
  va_list ap;
  va_start(ap, fmt);
  int n = vsnprintf(o.buf + o.len, o.cap - o.len, fmt, ap);
  va_end(ap);
  if (n < 0 || (size_t)n >= o.cap - o.len) {
    o.ok = false;
    return;
  }
  o.len += (size_t)n;
}

void put_string(Out &o, const char *s) {
  put(o, "\"");
  for (; s != 0 && *s != '\0'; ++s) {
    if (*s == '"' || *s == '\\') put(o, "\\%c", *s);
    else if ((unsigned char)*s < 0x20) put(o, "?");
    else put(o, "%c", *s);
  }
  put(o, "\"");
}

void key(Out &o, const char *k, bool first) {
  put(o, first ? "\"%s\":" : ",\"%s\":", k);
}

void kv_num(Out &o, const char *k, bool first, bool valid, double v, int decimals) {
  key(o, k, first);
  if (valid) put(o, "%.*f", decimals, v);
  else put(o, "null");
}

void kv_uint(Out &o, const char *k, bool first, bool valid, unsigned long v) {
  key(o, k, first);
  if (valid) put(o, "%lu", v);
  else put(o, "null");
}

void kv_int(Out &o, const char *k, bool first, bool valid, long v) {
  key(o, k, first);
  if (valid) put(o, "%ld", v);
  else put(o, "null");
}

void kv_bool(Out &o, const char *k, bool first, bool v) {
  key(o, k, first);
  put(o, v ? "true" : "false");
}

void kv_str(Out &o, const char *k, bool first, bool valid, const char *s) {
  key(o, k, first);
  if (valid && s != 0) put_string(o, s);
  else put(o, "null");
}

}

uint32_t telemetry_next_seq() {
  return ++seqCounter;
}

size_t telemetry_build_json(const TelemetryInput &in, char *buf, size_t cap) {
  if (buf == 0 || cap < 2) return 0;

  static const GnssFix emptyFix = {};
  static const ImuSample emptyImu = {};
  const GnssFix &g = in.gnss ? *in.gnss : emptyFix;
  const ImuSample &m = in.imu ? *in.imu : emptyImu;
  const DeviceConfig &c = config();

  Out o = {buf, cap, 0, true};
  buf[0] = '\0';

  put(o, "{");
  kv_str(o, "packet_version", true, true, c.packet_version);
  kv_str(o, "device_id", false, true, c.device_id);
  kv_uint(o, "seq", false, true, (unsigned long)in.seq);
  kv_uint(o, "uptime_ms", false, true, (unsigned long)in.uptime_ms);
  kv_str(o, "data_source", false, true, c.use_simulated_inputs ? "SIMULATED" : "HARDWARE");
  kv_str(o, "device_state", false, true, state_name(in.state));
  kv_str(o, "timestamp_utc", false, g.valid && g.time_valid, g.timestamp);

  key(o, "location", false);
  put(o, "{");
  kv_bool(o, "fix", true, g.valid);
  kv_num(o, "latitude", false, g.valid, g.latitude, 6);
  kv_num(o, "longitude", false, g.valid, g.longitude, 6);
  kv_num(o, "altitude_m", false, g.valid && g.altitude_valid, g.altitude_m, 1);
  kv_num(o, "speed_kmh", false, g.valid && g.speed_valid, g.speed_kmh, 1);
  kv_int(o, "satellites", false, g.satellites_valid, (long)g.satellites);
  kv_num(o, "hdop", false, g.valid && g.hdop_valid, g.hdop, 2);
  put(o, "}");

  key(o, "motion", false);
  put(o, "{");
  kv_num(o, "accel_x_g", true, m.valid, m.ax, 3);
  kv_num(o, "accel_y_g", false, m.valid, m.ay, 3);
  kv_num(o, "accel_z_g", false, m.valid, m.az, 3);
  kv_num(o, "gyro_x_dps", false, m.valid, m.gx, 1);
  kv_num(o, "gyro_y_dps", false, m.valid, m.gy, 1);
  kv_num(o, "gyro_z_dps", false, m.valid, m.gz, 1);
  kv_num(o, "accel_magnitude_g", false, m.valid, m.accel_mag_g, 3);
  put(o, "}");

  key(o, "event", false);
  put(o, "{");
  kv_str(o, "type", true, true, event_name(in.event));
  kv_bool(o, "sos_active", false, in.sos_active);
  put(o, "}");

  key(o, "comms", false);
  put(o, "{");
  kv_bool(o, "connected", true, in.comms_connected);
  put(o, "}");

  kv_uint(o, "battery_mv", false, in.battery_valid, (unsigned long)in.battery_mv);
  kv_uint(o, "fault_flags", false, true, (unsigned long)in.fault_flags);
  put(o, "}");

  if (!o.ok) {
    buf[0] = '\0';
    return 0;
  }
  return o.len;
}
