#include "event_builder.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

void epochToIso(uint32_t epoch, char* out, size_t size) {
  uint32_t days = epoch / 86400UL, rem = epoch % 86400UL;
  int hh = (int)(rem / 3600), mi = (int)((rem % 3600) / 60), ss = (int)(rem % 60);
  // civil date from days since 1970-01-01 (Howard Hinnant's algorithm)
  long z = (long)days + 719468L;
  long era = (z >= 0 ? z : z - 146096) / 146097;
  unsigned doe = (unsigned)(z - era * 146097);
  unsigned yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
  long y = (long)yoe + era * 400;
  unsigned doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
  unsigned mp = (5 * doy + 2) / 153;
  unsigned d = doy - (153 * mp + 2) / 5 + 1;
  unsigned m = mp < 10 ? mp + 3 : mp - 9;
  if (m <= 2) y++;
  snprintf(out, size, "%04ld-%02u-%02uT%02d:%02d:%02dZ", y, m, d, hh, mi, ss);
}

static bool put(char* out, size_t size, size_t& len, const char* fmt, ...) {
  if (len >= size) return false;
  va_list a; va_start(a, fmt);
  int n = vsnprintf(out + len, size - len, fmt, a);
  va_end(a);
  if (n < 0 || (size_t)n >= size - len) return false;
  len += (size_t)n;
  return true;
}

size_t buildEventJson(char* out, size_t size, const char* deviceId,
                      const EventData& e, const EventFormat& fmt) {
  if (!out || size == 0) return 0;
  size_t len = 0;
  out[0] = '\0';
  bool ok = true;
  const bool acc = (e.kind == EventKind::ACCIDENT);

  ok = ok && put(out, size, len, "{\"event_id\":\"%s\",\"device_id\":\"%s\",\"schema_version\":\"%s\",\"sequence_number\":%lu,",
                 e.eventId, deviceId, fmt.schemaVersion, (unsigned long)e.sequence);
  ok = ok && put(out, size, len, "\"event_type\":\"%s\",", acc ? "accident" : "sos");

  if (e.timeValid) {
    if (fmt.isoTimestamp) {
      char iso[24];
      epochToIso(e.epochUtc, iso, sizeof iso);
      ok = ok && put(out, size, len, "\"timestamp_utc\":\"%s\",", iso);
    } else {
      ok = ok && put(out, size, len, "\"timestamp_utc\":%lu,", (unsigned long)e.epochUtc);
    }
  } else {
    ok = ok && put(out, size, len, "\"timestamp_utc\":null,");
  }

  ok = ok && put(out, size, len, "\"description\":\"%s\",",
                 acc ? "Possible vehicle accident detected" : "SOS button pressed");

  if (e.locationValid) ok = ok && put(out, size, len, "\"latitude\":%.6f,\"longitude\":%.6f,", e.lat, e.lon);
  else                 ok = ok && put(out, size, len, "\"latitude\":null,\"longitude\":null,");

  if (e.locationAgeValid) ok = ok && put(out, size, len, "\"location_age_s\":%lu,", (unsigned long)e.locationAgeS);
  else                    ok = ok && put(out, size, len, "\"location_age_s\":null,");

  ok = ok && put(out, size, len, "\"gnss_fix_quality\":%u,", (unsigned)e.fixQuality);

  if (acc) ok = ok && put(out, size, len, "\"accident_type\":\"%s\",\"confirmation_state\":\"%s\",",
                          e.accidentType ? e.accidentType : "impact",
                          e.confirmationState ? e.confirmationState : "confirmed");

  ok = ok && put(out, size, len, "\"snapshot\":null");
  if (fmt.includeSimulatedFlag) ok = ok && put(out, size, len, ",\"simulated\":true");
  ok = ok && put(out, size, len, "}");

  if (!ok) { out[0] = '\0'; return 0; }
  return len;
}
