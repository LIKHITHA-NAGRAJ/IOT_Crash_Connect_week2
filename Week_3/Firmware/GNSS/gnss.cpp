#include "gnss.h"
#include "config.h"
#include <Arduino.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

namespace GNSS {

  // RECORDED / SAMPLE NMEA (public example sentences, NOT live GPS).
  static const char* const SAMPLE_LOG[] = {
    "$GPGGA,123520,,,,,0,00,,,M,,M,,*61",
    "$GPRMC,123520,V,,,,,,,230394,,,N*5B",
    "$GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47",
    "$GPRMC,123519,A,4807.038,N,01131.000,E,022.4,084.4,230394,003.1,W*6A"
  };
  static const int SAMPLE_COUNT = sizeof(SAMPLE_LOG) / sizeof(SAMPLE_LOG[0]);
  static int nextLine = 0;

  // Live GNSS input does not exist yet, so time is never "live".
  // A real UART path would set this to true. NOT IMPLEMENTED.
  static const bool SOURCE_IS_LIVE = false;

  // Latest values seen in each sentence type
  static bool     rmcValid;
  static int      ggaQuality;
  static uint8_t  sats;
  static bool     hdopValid;
  static float    hdop;
  static double   rmcLat, rmcLon, rmcSpeed, rmcCourse;
  static bool     rmcSpeedValid, rmcCourseValid, rmcUtcValid;
  static uint32_t rmcEpoch;
  static char     rmcIso[40];

  // Snapshot taken only when ALL validity checks passed
  static bool     currentOk;          // do the latest sentences describe a usable fix?
  static bool     haveSnapshot;
  static uint32_t lastValidMs;
  static double   snapLat, snapLon, snapSpeed, snapCourse;
  static bool     snapSpeedValid, snapCourseValid, snapUtcValid;
  static uint8_t  snapQuality;
  static uint32_t snapEpoch;
  static char     snapIso[40];

  static bool checksumOk(const char* line) {
    if (line[0] != '$') return false;
    const char* star = strchr(line, '*');
    if (!star) return false;
    uint8_t c = 0;
    for (const char* p = line + 1; p < star; p++) c ^= (uint8_t)*p;
    return c == (uint8_t)strtol(star + 1, NULL, 16);
  }

  static double toDecimal(const char* field, char hemi) {
    double v = atof(field);
    int deg = (int)(v / 100);
    double dec = deg + (v - deg * 100) / 60.0;
    return (hemi == 'S' || hemi == 'W') ? -dec : dec;
  }

  static int splitFields(char* buf, char* fields[], int maxFields) {
    int n = 0;
    fields[n++] = buf;
    for (char* p = buf; *p && n < maxFields; p++) {
      if (*p == ',' || *p == '*') { *p = '\0'; fields[n++] = p + 1; }
    }
    return n;
  }

  // Civil date to integer UTC epoch seconds.
  static uint32_t toEpoch(int y, int m, int d, int hh, int mm, int ss) {
    y -= (m <= 2);
    int era = (y >= 0 ? y : y - 399) / 400;
    unsigned yoe = (unsigned)(y - era * 400);
    unsigned doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
    unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    long days = (long)era * 146097L + (long)doe - 719468L;
    return (uint32_t)(days * 86400L + hh * 3600L + mm * 60L + ss);
  }

  static bool qualityOk() {
    return sats >= CONFIG.gnss.minSatellites &&
           (!hdopValid || hdop <= CONFIG.gnss.maxHdop);
  }

  // Called after every GGA or RMC sentence.
  static void evaluate() {
    currentOk = rmcValid && ggaQuality > 0 && qualityOk();
    if (currentOk) {
      snapLat = rmcLat; snapLon = rmcLon;
      snapSpeed = rmcSpeed; snapSpeedValid = rmcSpeedValid;
      snapCourse = rmcCourse; snapCourseValid = rmcCourseValid;
      snapUtcValid = rmcUtcValid; snapEpoch = rmcEpoch;
      snapQuality = (uint8_t)ggaQuality;
      strncpy(snapIso, rmcIso, sizeof snapIso - 1);
      snapIso[sizeof snapIso - 1] = '\0';
      lastValidMs = millis();
      haveSnapshot = true;
    }
  }

  void feedLine(const char* raw) {
    if (!checksumOk(raw)) {
      Serial.println("[GNSS] line rejected: bad checksum");
      return;
    }
    char buf[100];
    strncpy(buf, raw + 1, sizeof buf - 1);
    buf[sizeof buf - 1] = '\0';
    char* f[20];
    int n = splitFields(buf, f, 20);

    if (strcmp(f[0] + 2, "GGA") == 0 && n >= 9) {
      ggaQuality = atoi(f[6]);                       // 0 = no fix, 1 = GPS, 2 = DGPS, ...
      sats = (uint8_t)atoi(f[7]);
      hdopValid = (f[8][0] != '\0');
      hdop = hdopValid ? (float)atof(f[8]) : 0.0f;
    } else if (strcmp(f[0] + 2, "RMC") == 0 && n >= 10) {
      rmcValid = (f[2][0] == 'A');
      rmcSpeedValid = false; rmcCourseValid = false; rmcUtcValid = false;
      if (rmcValid && strlen(f[1]) >= 6 && strlen(f[9]) >= 6) {
        rmcLat = toDecimal(f[3], f[4][0]);
        rmcLon = toDecimal(f[5], f[6][0]);
        if (f[7][0] != '\0') { rmcSpeed = atof(f[7]); rmcSpeedValid = true; }
        if (f[8][0] != '\0') { rmcCourse = atof(f[8]); rmcCourseValid = true; }
        int dd = (f[9][0] - '0') * 10 + (f[9][1] - '0');
        int mo = (f[9][2] - '0') * 10 + (f[9][3] - '0');
        int yy = (f[9][4] - '0') * 10 + (f[9][5] - '0');
        int hh = (f[1][0] - '0') * 10 + (f[1][1] - '0');
        int mi = (f[1][2] - '0') * 10 + (f[1][3] - '0');
        int ss = (f[1][4] - '0') * 10 + (f[1][5] - '0');
        int year = (yy < 70) ? 2000 + yy : 1900 + yy;
        rmcEpoch = toEpoch(year, mo, dd, hh, mi, ss);
        rmcUtcValid = true;
        snprintf(rmcIso, sizeof rmcIso, "%04d-%02d-%02dT%02d:%02d:%02dZ", year, mo, dd, hh, mi, ss);
      }
    } else {
      return;                                        // not GGA or RMC: ignore
    }
    evaluate();
  }

  bool begin() {
    rmcValid = false; ggaQuality = 0; sats = 0; hdopValid = false; hdop = 0;
    rmcLat = rmcLon = rmcSpeed = rmcCourse = 0; rmcSpeedValid = rmcCourseValid = rmcUtcValid = false; rmcEpoch = 0; rmcIso[0] = '\0';
    currentOk = false; haveSnapshot = false; lastValidMs = 0;
    snapLat = snapLon = snapSpeed = snapCourse = 0; snapSpeedValid = snapCourseValid = snapUtcValid = false;
    snapQuality = 0; snapEpoch = 0; snapIso[0] = '\0';
    nextLine = 0;
    Serial.println("[GNSS] SIMULATED module initialized (recorded NMEA for parser tests, NOT live GPS)");
    return true;
  }

  void update() {
    feedLine(SAMPLE_LOG[nextLine]);
    nextLine++;
    if (nextLine >= SAMPLE_COUNT) nextLine = SAMPLE_COUNT - 2;   // keep repeating the valid pair
  }

  bool getFix(Fix& out) {
    memset(&out, 0, sizeof out);
    out.satellites = sats;
    out.hdopValid = hdopValid;
    out.hdop = hdop;
    out.ageMs = haveSnapshot ? (uint32_t)(millis() - lastValidMs) : 0xFFFFFFFFu;
    out.timeIsLive = SOURCE_IS_LIVE;

    bool fresh = haveSnapshot && out.ageMs <= CONFIG.timeouts.gnssStaleMs;
    if (!(currentOk && fresh)) {
      // No current fix: quality 0, and no position or speed is reported.
      // The old coordinates are never returned as the current location.
      out.valid = false;
      out.fixQuality = 0;
      return false;
    }
    out.valid = true;
    out.fixQuality = snapQuality;
    out.lat = snapLat;
    out.lon = snapLon;
    out.speedValid = snapSpeedValid;
    out.speedKnots = snapSpeed;
    out.courseValid = snapCourseValid;
    out.courseDeg = snapCourse;
    out.utcValid = snapUtcValid;
    out.epochUtc = snapEpoch;
    strncpy(out.time, snapIso, sizeof out.time - 1);
    return true;
  }

  const char* statusText(const Fix& f) {
    return f.valid ? "VALID" : "NO_FIX";   // NO_FIX is reported whenever the validity criteria are not met
  }

  // HISTORICAL data. Must never be presented as the current location.
  bool getLastValid(LastValid& out) {
    memset(&out, 0, sizeof out);
    if (!haveSnapshot) return false;
    out.have = true;
    out.lat = snapLat;
    out.lon = snapLon;
    out.utcValid = snapUtcValid;
    out.epochUtc = snapEpoch;
    out.ageMs = (uint32_t)(millis() - lastValidMs);
    return true;
  }
}
