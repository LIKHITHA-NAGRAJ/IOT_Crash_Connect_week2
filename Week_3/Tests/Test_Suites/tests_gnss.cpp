#include <Arduino.h>
#include <string.h>
#include "config.h"
#include "gnss.h"
#include "tests.h"

// GNSS parser and validity tests.
// INPUT: RECORDED sample NMEA lines (public examples) fed to the parser.
// These are parser tests only. The 1994 sample time is NOT live device time, and
// nothing here tests a real GNSS receiver.

static int passed = 0, failed = 0;

static void check(bool ok, const char* label) {
  Serial.print(ok ? "  PASS: " : "  FAIL: ");
  Serial.println(label);
  if (ok) passed++; else failed++;
}

// Recorded sample lines (checksums included)
static const char* GGA_OK      = "$GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47";
static const char* RMC_OK      = "$GPRMC,123519,A,4807.038,N,01131.000,E,022.4,084.4,230394,003.1,W*6A";
static const char* GGA_NOFIX   = "$GPGGA,123520,,,,,0,00,,,M,,M,,*61";
static const char* RMC_NOFIX   = "$GPRMC,123520,V,,,,,,,230394,,,N*5B";
static const char* GGA_3SATS   = "$GPGGA,123519,4807.038,N,01131.000,E,1,03,1.2,545.4,M,46.9,M,,*46";
static const char* GGA_BADHDOP = "$GPGGA,123519,4807.038,N,01131.000,E,1,08,9.9,545.4,M,46.9,M,,*4E";
static const char* RMC_BADSUM  = "$GPRMC,123519,A,4807.038,N,01131.000,E,022.4,084.4,230394,003.1,W*00";

int runGnssTests() {
  Serial.println("\n=== GNSS WEEK 3 TESTS (recorded NMEA, parser tests only) ===");
  Serial.print("Placeholder limits: min satellites = "); Serial.print(CONFIG.gnss.minSatellites);
  Serial.print(", max HDOP = "); Serial.print(CONFIG.gnss.maxHdop, 1);
  Serial.print(", stale after (ms) = "); Serial.println(CONFIG.timeouts.gnssStaleMs);

  GNSS::Fix f;

  Serial.println("\nTest 1: valid recorded pair");
  GNSS::begin(); GNSS::feedLine(GGA_OK); GNSS::feedLine(RMC_OK); GNSS::getFix(f);
  check(f.valid, "fix is valid");
  check(f.fixQuality == 1, "fix quality comes from GGA (1)");
  check(f.satellites == 8 && f.hdopValid && f.hdop < 1.0f, "satellites 8 and HDOP 0.9 parsed");
  check(f.speedValid && f.speedKnots > 22.3 && f.speedKnots < 22.5, "speed 22.4 knots parsed");
  check(f.utcValid && f.epochUtc == 764426119UL, "UTC epoch seconds computed (764426119)");
  check(f.courseValid && f.courseDeg > 84.3 && f.courseDeg < 84.5, "course 84.4 degrees parsed");
  check(strcmp(GNSS::statusText(f), "VALID") == 0, "status text is VALID");
  check(!f.timeIsLive, "time flagged NOT live (recorded data)");
  Serial.print("  lat="); Serial.print(f.lat, 6);
  Serial.print(" lon="); Serial.print(f.lon, 6);
  Serial.print(" age(ms)="); Serial.println(f.ageMs);

  Serial.println("\nTest 2: recorded no-fix pair");
  GNSS::begin(); GNSS::feedLine(GGA_NOFIX); GNSS::feedLine(RMC_NOFIX); GNSS::getFix(f);
  check(!f.valid, "fix not valid");
  check(f.fixQuality == 0, "fix quality 0");
  check(!f.speedValid && !f.utcValid, "speed and UTC not valid");
  check(f.ageMs == 0xFFFFFFFFu, "age = never had a fix");
  check(strcmp(GNSS::statusText(f), "NO_FIX") == 0, "status text is NO_FIX");
  check(!f.courseValid, "course not valid");
  GNSS::LastValid lv0;
  check(!GNSS::getLastValid(lv0), "no last-valid fix exists");

  Serial.println("\nTest 3: too few satellites (3 < placeholder minimum)");
  GNSS::begin(); GNSS::feedLine(GGA_3SATS); GNSS::feedLine(RMC_OK); GNSS::getFix(f);
  check(!f.valid && f.fixQuality == 0, "rejected, quality 0");

  Serial.println("\nTest 4: HDOP too high (9.9 > placeholder maximum)");
  GNSS::begin(); GNSS::feedLine(GGA_BADHDOP); GNSS::feedLine(RMC_OK); GNSS::getFix(f);
  check(!f.valid && f.fixQuality == 0, "rejected, quality 0");

  Serial.println("\nTest 5: corrupted line (bad checksum)");
  GNSS::begin(); GNSS::feedLine(GGA_OK); GNSS::feedLine(RMC_BADSUM); GNSS::getFix(f);
  check(!f.valid, "no fix: corrupted RMC was rejected");

  Serial.println("\nTest 6: fix becomes old -> must not be shown as current");
  GNSS::begin(); GNSS::feedLine(GGA_OK); GNSS::feedLine(RMC_OK); GNSS::getFix(f);
  check(f.valid, "valid right after the lines");
  delay(CONFIG.timeouts.gnssStaleMs + 500);
  GNSS::getFix(f);
  check(!f.valid && f.fixQuality == 0, "after the stale limit: not valid, quality 0");
  check(f.ageMs > CONFIG.timeouts.gnssStaleMs && f.ageMs != 0xFFFFFFFFu, "age of last valid fix is reported");
  check(strcmp(GNSS::statusText(f), "NO_FIX") == 0, "status text is NO_FIX when stale");
  GNSS::LastValid lv;
  check(GNSS::getLastValid(lv) && lv.have && lv.ageMs > CONFIG.timeouts.gnssStaleMs, "last-valid fix kept as HISTORY with its age");
  check(lv.lat > 48.11 && lv.lat < 48.12, "history is available, but fix.valid stays false (not current)");

  Serial.println("\nTest 7: valid fix lost -> no-fix sentences arrive");
  GNSS::begin(); GNSS::feedLine(GGA_OK); GNSS::feedLine(RMC_OK);
  GNSS::feedLine(GGA_NOFIX); GNSS::feedLine(RMC_NOFIX); GNSS::getFix(f);
  check(!f.valid && f.fixQuality == 0 && !f.speedValid, "reported as no fix immediately");

  Serial.println("\nTest 8: GPS loss and recovery");
  GNSS::begin(); GNSS::feedLine(GGA_OK); GNSS::feedLine(RMC_OK); GNSS::getFix(f);
  check(f.valid, "step 1: valid fix");
  GNSS::feedLine(GGA_NOFIX); GNSS::feedLine(RMC_NOFIX); GNSS::getFix(f);
  check(!f.valid && f.fixQuality == 0, "step 2: fix lost -> NO_FIX, quality 0");
  GNSS::LastValid lvR;
  check(GNSS::getLastValid(lvR) && lvR.have, "step 2: old fix kept only as history");
  delay(1000);
  GNSS::feedLine(GGA_OK); GNSS::feedLine(RMC_OK); GNSS::getFix(f);
  check(f.valid && f.fixQuality == 1, "step 3: fix recovered");
  check(f.ageMs < 100, "step 3: age reset after the new valid fix");

  Serial.println("\nTest 9: malformed / invalid input must not crash or create a fix");
  GNSS::begin();
  GNSS::feedLine("");
  GNSS::feedLine("hello world");
  GNSS::feedLine("$GPRMC,123519,A");
  GNSS::feedLine("$GPGGA,123519*77");
  GNSS::getFix(f);
  check(!f.valid && f.fixQuality == 0, "garbage, truncated and too-short lines ignored; NO_FIX");

  Serial.print("\nRESULT: "); Serial.print(passed); Serial.print(" passed, ");
  Serial.print(failed); Serial.println(" failed");
  Serial.println("NOTE: recorded data, parser tests only. No live GPS, no hardware.");
  return failed;
}
