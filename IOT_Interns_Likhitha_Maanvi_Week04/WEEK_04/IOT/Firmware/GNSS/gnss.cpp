#include <Arduino.h>
#include <TinyGPS++.h>
#include <stdlib.h>
#include <string.h>
#include "gnss.h"
#include "../Config/config.h"
#include "../Device/device.h"

namespace {

TinyGPSPlus gps;
TinyGPSCustom ggaGP(gps, "GPGGA", 6);
TinyGPSCustom ggaGN(gps, "GNGGA", 6);
TinyGPSCustom rmcGP(gps, "GPRMC", 2);
TinyGPSCustom rmcGN(gps, "GNRMC", 2);

GnssState st;
int fixQuality = 0;
bool rmcSeen = false;
bool rmcActive = false;
bool haveSentence = false;
uint32_t lastGoodMs = 0;
uint32_t lastPassed = 0;

}

bool gnss_init() {
  memset(&st, 0, sizeof(st));
  st.status = GNSS_NO_DATA;
  fixQuality = 0;
  rmcSeen = false;
  rmcActive = false;
  haveSentence = false;
  lastPassed = gps.passedChecksum();
  return true;
}

const char *gnss_status_name(GnssStatus s) {
  switch (s) {
    case GNSS_FIX:    return "FIX";
    case GNSS_NO_FIX: return "NO_FIX";
    case GNSS_STALE:  return "STALE";
    default:          return "NO_DATA";
  }
}

void gnss_feed(const char *nmea) {
  if (nmea == 0) return;
  for (const char *p = nmea; *p; ++p) gps.encode(*p);
  if (gps.passedChecksum() > lastPassed) {
    lastPassed = gps.passedChecksum();
    lastGoodMs = millis();
    haveSentence = true;
  }
  if (ggaGP.isUpdated()) fixQuality = atoi(ggaGP.value());
  if (ggaGN.isUpdated()) fixQuality = atoi(ggaGN.value());
  if (rmcGP.isUpdated()) {
    rmcSeen = true;
    rmcActive = rmcGP.value()[0] == 'A';
  }
  if (rmcGN.isUpdated()) {
    rmcSeen = true;
    rmcActive = rmcGN.value()[0] == 'A';
  }
}

void gnss_update(uint32_t now_ms) {
  const uint32_t stale = timing().gnss_stale_ms;
  st.fix_quality = fixQuality;
  st.sats_valid = gps.satellites.isValid();
  st.satellites = st.sats_valid ? (int)gps.satellites.value() : 0;
  memset(&st.current, 0, sizeof(st.current));

  if (!haveSentence) {
    st.status = GNSS_NO_DATA;
    return;
  }
  if ((now_ms - lastGoodMs) > stale) {
    st.status = GNSS_STALE;
    return;
  }
  bool fix = fixQuality > 0 && (!rmcSeen || rmcActive) && gps.location.isValid() && gps.location.age() <= stale;
  if (!fix) {
    st.status = GNSS_NO_FIX;
    return;
  }
  st.status = GNSS_FIX;
  GnssFix &c = st.current;
  c.valid = true;
  c.fix_quality = fixQuality;
  c.latitude = gps.location.lat();
  c.longitude = gps.location.lng();
  c.fix_ms = now_ms - (uint32_t)gps.location.age();
  if (gps.speed.isValid() && gps.speed.age() <= stale) {
    c.speed_valid = true;
    c.speed_kmh = (float)gps.speed.kmph();
  }
  if (gps.course.isValid() && gps.course.age() <= stale) {
    c.course_valid = true;
    c.course_deg = (float)gps.course.deg();
  }
  if (gps.hdop.isValid()) {
    c.hdop_valid = true;
    c.hdop = (float)gps.hdop.hdop();
  }
  c.sats_valid = st.sats_valid;
  c.satellites = st.satellites;
  if (gps.date.isValid() && gps.time.isValid() && gps.date.year() >= 2024 && gps.date.year() <= 2060) {
    c.utc_valid = true;
    c.utc_epoch = epoch_from_ymdhms(gps.date.year(), gps.date.month(), gps.date.day(), gps.time.hour(),
                                    gps.time.minute(), gps.time.second());
  }
  st.has_last_valid = true;
  st.last_valid = c;
}

const GnssState &gnss_state() {
  return st;
}

uint32_t gnss_last_valid_age_s(uint32_t now_ms) {
  if (!st.has_last_valid) return 0;
  return (now_ms - st.last_valid.fix_ms) / 1000u;
}

uint32_t gnss_checksum_failures() {
  return gps.failedChecksum();
}
