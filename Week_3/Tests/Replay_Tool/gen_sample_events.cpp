// Host tool: writes SIMULATED sample event payloads (valid and invalid) for API testing with Full Stack.
// Build (from this folder):
//   g++ -std=c++17 -I ../../Firmware/SOS_Events gen_sample_events.cpp ../../Firmware/SOS_Events/event_builder.cpp -o gen_sample_events
// Run:
//   ./gen_sample_events ../../Test_Data/Sample_Events
// ALL VALUES ARE MOCK. Coordinates come from a recorded NMEA sample, not live GPS. Nothing is transmitted.
#include <stdio.h>
#include <string.h>
#include "event_builder.h"

static void writeFile(const char* dir, const char* name, const char* json) {
  char path[512];
  snprintf(path, sizeof path, "%s/%s", dir, name);
  FILE* f = fopen(path, "w");
  if (!f) { fprintf(stderr, "cannot write %s\n", path); return; }
  fprintf(f, "%s\n", json);
  fclose(f);
  printf("wrote %s\n", path);
}

static void replaceOnce(char* buf, size_t size, const char* from, const char* to) {
  char* p = strstr(buf, from);
  if (!p) return;
  char tmp[600];
  snprintf(tmp, sizeof tmp, "%.*s%s%s", (int)(p - buf), buf, to, p + strlen(from));
  strncpy(buf, tmp, size - 1);
  buf[size - 1] = '\0';
}

static EventData base(const char* id, uint32_t seq, EventKind k) {
  EventData e = {};
  snprintf(e.eventId, sizeof e.eventId, "%s", id);
  e.sequence = seq; e.kind = k;
  e.timeValid = true; e.epochUtc = 1792582500UL;           // 2026-10-21T11:35:00Z (mock)
  e.locationValid = true; e.lat = 48.1173; e.lon = 11.516667;
  e.locationAgeValid = true; e.locationAgeS = 0; e.fixQuality = 1;
  e.accidentType = "impact"; e.confirmationState = "confirmed";
  return e;
}

int main(int argc, char** argv) {
  const char* dir = (argc > 1) ? argv[1] : ".";
  EventFormat epoch = { "1.0", false, true };
  EventFormat iso   = { "1.0", true,  true };
  char j[600], bad[600];

  EventData e = base("CC-IOT-01-B0001-000001", 1, EventKind::SOS);
  buildEventJson(j, sizeof j, "CC-IOT-01", e, epoch);  writeFile(dir, "valid_sos_current_location.json", j);
  buildEventJson(j, sizeof j, "CC-IOT-01", e, iso);    writeFile(dir, "valid_sos_iso_timestamp.json", j);

  e = base("CC-IOT-01-B0001-000002", 2, EventKind::SOS);
  e.locationAgeS = 45; e.fixQuality = 0;                // GPS lost 45 s ago: last-known location with its age
  buildEventJson(j, sizeof j, "CC-IOT-01", e, epoch);  writeFile(dir, "valid_sos_last_known_location.json", j);

  e = base("CC-IOT-01-B0001-000003", 3, EventKind::SOS);
  e.timeValid = false; e.locationValid = false; e.locationAgeValid = false; e.fixQuality = 0;
  buildEventJson(j, sizeof j, "CC-IOT-01", e, epoch);  writeFile(dir, "valid_sos_no_location_no_time.json", j);

  e = base("CC-IOT-01-B0001-000004", 4, EventKind::ACCIDENT);
  buildEventJson(j, sizeof j, "CC-IOT-01", e, epoch);  writeFile(dir, "valid_accident_impact_confirmed.json", j);

  e = base("CC-IOT-01-B0001-000005", 5, EventKind::ACCIDENT);
  e.accidentType = "rollover_suspected"; e.confirmationState = "confirmed_low_confidence";
  buildEventJson(j, sizeof j, "CC-IOT-01", e, epoch);  writeFile(dir, "valid_accident_rollover_low_confidence.json", j);

  e = base("CC-IOT-01-B0001-000006", 6, EventKind::SOS);
  buildEventJson(j, sizeof j, "CC-IOT-01", e, epoch);
  strcpy(bad, j); replaceOnce(bad, sizeof bad, "\"event_id\":", "\"event_idX\":");
  writeFile(dir, "invalid_missing_event_id.json", bad);
  strcpy(bad, j); replaceOnce(bad, sizeof bad, "\"sos\"", "\"fire\"");
  writeFile(dir, "invalid_unknown_event_type.json", bad);
  strcpy(bad, j); replaceOnce(bad, sizeof bad, "\"latitude\":48.117300", "\"latitude\":95.000000");
  writeFile(dir, "invalid_latitude_out_of_range.json", bad);
  strcpy(bad, j); replaceOnce(bad, sizeof bad, "\"timestamp_utc\":1792582500", "\"timestamp_utc\":\"2026-10-21T11:35:00Z\"");
  writeFile(dir, "invalid_timestamp_text_when_integer_expected.json", bad);
  strcpy(bad, j); replaceOnce(bad, sizeof bad, "\"longitude\":11.516667", "\"longitude\":null");
  writeFile(dir, "invalid_latitude_without_longitude.json", bad);
  strcpy(bad, j); replaceOnce(bad, sizeof bad, "\"gnss_fix_quality\":1", "\"gnss_fix_quality\":\"valid\"");
  writeFile(dir, "invalid_fix_quality_text_when_integer_expected.json", bad);
  return 0;
}
