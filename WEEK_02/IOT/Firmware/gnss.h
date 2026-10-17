#pragma once

#include <stdbool.h>
#include <stdint.h>

// GNSS module: parses NMEA sentences (GGA + RMC) and reports a fix.
// INPUT for now: RECORDED / SAMPLE NMEA lines (NOT live GPS).
// Real UART reading from the u-blox MAX-M10S: NOT IMPLEMENTED.

namespace GNSS {
  struct Fix {
    bool    valid;       // false = no usable fix; lat/lon must be ignored
    double  lat;         // decimal degrees (double: float is too coarse for 6 decimals)
    double  lon;
    char    time[24];    // ISO 8601 UTC, e.g. 1994-03-23T12:35:19Z
    uint8_t satellites;
  };

  bool begin();
  void update();              // feed the next recorded NMEA line to the parser
  bool getFix(Fix& out);      // returns fix.valid
}