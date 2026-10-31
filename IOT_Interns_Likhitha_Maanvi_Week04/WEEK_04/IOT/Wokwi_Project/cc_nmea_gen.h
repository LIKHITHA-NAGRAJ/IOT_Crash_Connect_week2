#pragma once
#include <stdio.h>
#include <string.h>

inline void nmea_wrap(const char *body, char *out, size_t cap) {
  unsigned cs = 0;
  for (const char *p = body; *p; ++p) cs ^= (unsigned char)*p;
  snprintf(out, cap, "$%s*%02X\r\n", body, cs);
}

inline void nmea_pair(int sec_of_day, bool fix, double speed_kmh, char *out, size_t cap) {
  int h = (sec_of_day / 3600) % 24, m = (sec_of_day / 60) % 60, s = sec_of_day % 60;
  char gga[120], rmc[140], a[160], b[180];
  if (fix) {
    snprintf(gga, sizeof(gga), "GPGGA,%02d%02d%02d.00,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,", h, m, s);
    snprintf(rmc, sizeof(rmc), "GPRMC,%02d%02d%02d.00,A,4807.038,N,01131.000,E,%05.1f,084.4,311026,,,A", h, m, s,
             speed_kmh / 1.852);
  } else {
    snprintf(gga, sizeof(gga), "GPGGA,%02d%02d%02d.00,,,,,0,03,99.99,,,,,,", h, m, s);
    snprintf(rmc, sizeof(rmc), "GPRMC,%02d%02d%02d.00,V,,,,,,,311026,,,N", h, m, s);
  }
  nmea_wrap(gga, a, sizeof(a));
  nmea_wrap(rmc, b, sizeof(b));
  snprintf(out, cap, "%s%s", a, b);
}
