#pragma once
#include <stdint.h>

struct GnssFix {
  bool valid;
  int fix_quality;
  double latitude;
  double longitude;
  bool altitude_valid;
  float altitude_m;
  bool speed_valid;
  float speed_kmh;
  bool hdop_valid;
  float hdop;
  bool satellites_valid;
  int satellites;
  bool time_valid;
  char timestamp[40];
};

bool gnss_init();
void gnss_poll_uart();
void gnss_feed_nmea(const char *sentences);
bool gnss_get_fix(GnssFix *out, uint32_t now_ms);
uint32_t gnss_checksum_failures();
bool gnss_module_ok();
