#include <Arduino.h>
#include <TinyGPS++.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gnss.h"
#include "../config/config.h"

namespace {

TinyGPSPlus gps;
TinyGPSCustom ggaFixGP(gps, "GPGGA", 6);
TinyGPSCustom ggaFixGN(gps, "GNGGA", 6);

bool initialised = false;
bool haveGoodSentence = false;
int fixQuality = 0;
uint32_t lastGoodMs = 0;
uint32_t lastPassedCount = 0;
bool uartWarned = false;

}

bool gnss_init() {
  fixQuality = 0;
  haveGoodSentence = false;
  lastGoodMs = 0;
  lastPassedCount = gps.passedChecksum();
  initialised = true;
  return true;
}

void gnss_poll_uart() {
  if (config().use_simulated_inputs) return;
  if (!uartWarned) {
    Serial.println("[GNSS][STUB] UART read not implemented (MAX-M10S pins/baud: PENDING CONFIRMATION)");
    uartWarned = true;
  }
}

void gnss_feed_nmea(const char *sentences) {
  if (sentences == 0) return;
  for (const char *p = sentences; *p != '\0'; ++p) {
    gps.encode(*p);
  }
  if (gps.passedChecksum() > lastPassedCount) {
    lastPassedCount = gps.passedChecksum();
    lastGoodMs = millis();
    haveGoodSentence = true;
  }
  if (ggaFixGP.isUpdated()) fixQuality = atoi(ggaFixGP.value());
  if (ggaFixGN.isUpdated()) fixQuality = atoi(ggaFixGN.value());
}

bool gnss_get_fix(GnssFix *out, uint32_t now_ms) {
  memset(out, 0, sizeof(*out));
  out->fix_quality = fixQuality;

  bool fresh = haveGoodSentence && (now_ms - lastGoodMs) <= config().timing.gnss_stale_ms;
  if (!fresh) return false;

  if (gps.satellites.isValid()) {
    out->satellites_valid = true;
    out->satellites = (int)gps.satellites.value();
  }

  if (fixQuality <= 0 || !gps.location.isValid()) return false;

  out->valid = true;
  out->latitude = gps.location.lat();
  out->longitude = gps.location.lng();

  if (gps.altitude.isValid()) {
    out->altitude_valid = true;
    out->altitude_m = (float)gps.altitude.meters();
  }
  if (gps.speed.isValid()) {
    out->speed_valid = true;
    out->speed_kmh = (float)gps.speed.kmph();
  }
  if (gps.hdop.isValid()) {
    out->hdop_valid = true;
    out->hdop = (float)gps.hdop.hdop();
  }
  if (gps.date.isValid() && gps.time.isValid()) {
    int year = gps.date.year();
    if (year >= 2080) year -= 100;
    snprintf(out->timestamp, sizeof(out->timestamp), "%04d-%02d-%02dT%02d:%02d:%02dZ",
             year, (int)gps.date.month(), (int)gps.date.day(),
             (int)gps.time.hour(), (int)gps.time.minute(), (int)gps.time.second());
    out->time_valid = true;
  }
  return true;
}

uint32_t gnss_checksum_failures() {
  return gps.failedChecksum();
}

bool gnss_module_ok() {
  return initialised;
}
