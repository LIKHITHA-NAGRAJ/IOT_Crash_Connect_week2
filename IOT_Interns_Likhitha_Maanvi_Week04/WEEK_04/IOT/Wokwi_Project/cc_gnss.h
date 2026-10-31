#pragma once
#include <stdint.h>

enum GnssStatus : uint8_t {
  GNSS_NO_DATA = 0,
  GNSS_NO_FIX,
  GNSS_FIX,
  GNSS_STALE
};

struct GnssFix {
  bool valid;
  int fix_quality;
  double latitude;
  double longitude;
  bool speed_valid;
  float speed_kmh;
  bool course_valid;
  float course_deg;
  bool hdop_valid;
  float hdop;
  bool sats_valid;
  int satellites;
  bool utc_valid;
  uint32_t utc_epoch;
  uint32_t fix_ms;
};

struct GnssState {
  GnssStatus status;
  int fix_quality;
  int satellites;
  bool sats_valid;
  GnssFix current;
  bool has_last_valid;
  GnssFix last_valid;
};

bool gnss_init();
void gnss_feed(const char *nmea);
void gnss_update(uint32_t now_ms);
const GnssState &gnss_state();
uint32_t gnss_last_valid_age_s(uint32_t now_ms);
uint32_t gnss_checksum_failures();
const char *gnss_status_name(GnssStatus s);
