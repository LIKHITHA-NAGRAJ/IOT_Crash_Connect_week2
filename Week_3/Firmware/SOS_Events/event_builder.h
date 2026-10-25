#pragma once

#include <stddef.h>
#include <stdint.h>

// Event payload builder for accident and SOS events (flat JSON).
// Base fields follow the sample event model from Full Stack (device_id, schema_version,
// sequence_number, event_type, timestamp_utc, description, latitude, longitude,
// gnss_fix_quality, snapshot). The extra fields are PROPOSED and need confirmation:
//   event_id, location_age_s, accident_type, confirmation_state, simulated
// Nothing here has been checked against the real backend contract.

enum class EventKind { SOS, ACCIDENT };

struct EventData {
  char     eventId[40];          // unique, stable across retries and reboots
  uint32_t sequence;             // per-device event counter
  EventKind kind;
  bool     timeValid;            // false -> timestamp_utc is null
  uint32_t epochUtc;             // integer UTC epoch seconds
  bool     locationValid;        // false -> latitude and longitude are null
  double   lat, lon;             // current fix, or the LAST KNOWN fix (see locationAgeS)
  bool     locationAgeValid;
  uint32_t locationAgeS;         // age of the location in seconds (0 for a current fix)
  uint8_t  fixQuality;           // quality of the CURRENT fix: 0 = no valid fix now
  const char* accidentType;      // accident only: "impact" or "rollover_suspected"
  const char* confirmationState; // accident only: "confirmed" or "confirmed_low_confidence"
};

struct EventFormat {
  const char* schemaVersion;
  bool isoTimestamp;             // false = integer epoch seconds, true = ISO 8601 text
  bool includeSimulatedFlag;     // adds "simulated":true (labelling)
};

// Writes JSON to out. Returns its length, or 0 if the buffer is too small.
size_t buildEventJson(char* out, size_t size, const char* deviceId,
                      const EventData& e, const EventFormat& fmt);

// Converts integer epoch seconds to "YYYY-MM-DDThh:mm:ssZ" (needs at least 21 bytes).
void epochToIso(uint32_t epoch, char* out, size_t size);
