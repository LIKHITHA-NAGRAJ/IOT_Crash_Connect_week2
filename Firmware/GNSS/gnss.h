#pragma once

#include <stdbool.h>
#include <stdint.h>

// GNSS module: parses NMEA (GGA + RMC), checks fix validity, tracks fix age.
// INPUT for now: RECORDED sample NMEA lines. They are for PARSER TESTS ONLY, and the
// sample timestamp is NOT live device time. Real UART reading is NOT IMPLEMENTED.

namespace GNSS {
  struct Fix {
    bool     valid;        // true only for a CURRENT, usable fix
    uint8_t  fixQuality;   // from GGA field 6 (0 = no valid fix). Never hard-coded.
    double   lat, lon;     // only meaningful when valid; otherwise report null
    bool     speedValid;   // speed from RMC; false when no valid fix
    double   speedKnots;   // speed over ground in knots (unit conversion is PENDING CONFIRMATION)
    bool     courseValid;  // course over ground from RMC; false when no valid fix
    double   courseDeg;    // degrees true, 0-360
    uint8_t  satellites;   // from GGA (latest sentence, for diagnostics)
    bool     hdopValid;    // HDOP reported by the receiver
    float    hdop;
    uint32_t ageMs;        // ms since the last valid fix; 0xFFFFFFFF = never had one
    bool     utcValid;     // epochUtc parsed from RMC date and time
    uint32_t epochUtc;     // integer UTC epoch seconds from the parsed sentence
    bool     timeIsLive;   // false for recorded data: must NOT be sent as current time
    char     time[24];     // ISO 8601 text, for display and parser tests only
  };

  // Last valid fix, kept ONLY as history. It is NOT the current location.
  struct LastValid {
    bool     have;         // false if there has never been a valid fix
    double   lat, lon;
    bool     utcValid;
    uint32_t epochUtc;     // integer UTC epoch seconds from that sentence
    uint32_t ageMs;        // how old it is
  };

  bool begin();
  void update();                    // feed the next recorded sample line
  void feedLine(const char* line);  // feed one NMEA line (PARSER TESTS with recorded data)
  bool getFix(Fix& out);            // returns fix.valid
  const char* statusText(const Fix& f);   // "VALID" or "NO_FIX"
  bool getLastValid(LastValid& out);      // HISTORICAL data only; check ageMs
}
