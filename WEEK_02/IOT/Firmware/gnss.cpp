#include "gnss.h"
#include <Arduino.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

namespace GNSS {

  // RECORDED / SAMPLE NMEA (public example sentences, NOT live GPS).
  // The first pair has NO fix, the second pair has a valid fix.
  static const char* const SAMPLE_LOG[] = {
    "$GPGGA,123520,,,,,0,00,,,M,,M,,*61",
    "$GPRMC,123520,V,,,,,,,230394,,,N*5B",
    "$GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47",
    "$GPRMC,123519,A,4807.038,N,01131.000,E,022.4,084.4,230394,003.1,W*6A"
  };
  static const int SAMPLE_COUNT = sizeof(SAMPLE_LOG) / sizeof(SAMPLE_LOG[0]);

  static Fix  current;
  static bool rmcValid = false;
  static int  ggaQuality = 0;
  static int  nextLine = 0;

  // XOR of the characters between '$' and '*' must equal the hex number after '*'
  static bool checksumOk(const char* line) {
    if (line[0] != '$') return false;
    const char* star = strchr(line, '*');
    if (!star) return false;
    uint8_t c = 0;
    for (const char* p = line + 1; p < star; p++) c ^= (uint8_t)*p;
    return c == (uint8_t)strtol(star + 1, NULL, 16);
  }

  // NMEA "ddmm.mmmm" -> decimal degrees. South and West are negative.
  static double toDecimal(const char* field, char hemi) {
    double v = atof(field);
    int deg = (int)(v / 100);
    double dec = deg + (v - deg * 100) / 60.0;
    return (hemi == 'S' || hemi == 'W') ? -dec : dec;
  }

  // Split a sentence at commas (modifies the buffer).
  static int splitFields(char* buf, char* fields[], int maxFields) {
    int n = 0;
    fields[n++] = buf;
    for (char* p = buf; *p && n < maxFields; p++) {
      if (*p == ',' || *p == '*') { *p = '\0'; fields[n++] = p + 1; }
    }
    return n;
  }

  static void handleLine(const char* raw) {
    if (!checksumOk(raw)) {
      Serial.println("[GNSS] line rejected: bad checksum");
      return;
    }
    char buf[100];
    strncpy(buf, raw + 1, sizeof buf - 1);
    buf[sizeof buf - 1] = '\0';
    char* f[20];
    int n = splitFields(buf, f, 20);

    if (strcmp(f[0] + 2, "GGA") == 0 && n >= 8) {
      ggaQuality = atoi(f[6]);                  // 0 = no fix
      current.satellites = (uint8_t)atoi(f[7]);
    } else if (strcmp(f[0] + 2, "RMC") == 0 && n >= 10) {
      rmcValid = (f[2][0] == 'A');              // A = valid, V = no fix
      if (rmcValid && strlen(f[1]) >= 6 && strlen(f[9]) >= 6) {
        current.lat = toDecimal(f[3], f[4][0]);
        current.lon = toDecimal(f[5], f[6][0]);
        int yy = (f[9][4] - '0') * 10 + (f[9][5] - '0');
        int year = (yy < 70) ? 2000 + yy : 1900 + yy;
        snprintf(current.time, sizeof current.time, "%04d-%c%c-%c%cT%c%c:%c%c:%c%cZ",
                 year, f[9][2], f[9][3], f[9][0], f[9][1],
                 f[1][0], f[1][1], f[1][2], f[1][3], f[1][4], f[1][5]);
      }
    } else {
      return;                                    // not GGA or RMC: ignore
    }
    // A fix is valid only if BOTH sentences say so.
    current.valid = rmcValid && ggaQuality > 0;
  }

  bool begin() {
    memset(&current, 0, sizeof current);         // starts as NO FIX
    rmcValid = false;
    ggaQuality = 0;
    nextLine = 0;
    Serial.println("[GNSS] SIMULATED module initialized (recorded NMEA, NOT live GPS)");
    return true;
  }

  void update() {
    handleLine(SAMPLE_LOG[nextLine]);
    nextLine++;
    if (nextLine >= SAMPLE_COUNT) nextLine = SAMPLE_COUNT - 2;   // keep repeating the valid pair
  }

  bool getFix(Fix& out) {
    out = current;
    return current.valid;
  }
}