#include "event_transport.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ---------------- MockTransport (SIMULATED backend) ----------------
void MockTransport::begin(Mode m, int n, bool isoTimestamp) {
  // Reset field by field. (memset on a class with a virtual function would erase its vtable pointer.)
  iso = isoTimestamp;
  nAttempts = nReceived = nUnique = nDuplicates = nRejected = 0;
  memset(uniqueIds, 0, sizeof uniqueIds);
  memset(lastPayloadBuf, 0, sizeof lastPayloadBuf);
  lastReason = nullptr;
  setMode(m, n);
}

void MockTransport::setMode(Mode m, int n) {
  mode = m;
  failRemaining = n;
  lostAckPending = (m == ACK_LOST_ONCE);
}

// Returns a pointer just after "key": in the JSON, or nullptr.
static const char* valueOf(const char* json, const char* key) {
  char pat[48];
  snprintf(pat, sizeof pat, "\"%s\":", key);
  const char* p = strstr(json, pat);
  return p ? p + strlen(pat) : nullptr;
}

bool MockTransport::validate(const char* json, const char** reason) const {
  static const char* required[] = { "event_id", "device_id", "event_type", "timestamp_utc",
                                    "latitude", "longitude", "gnss_fix_quality" };
  for (unsigned i = 0; i < sizeof required / sizeof required[0]; i++)
    if (!valueOf(json, required[i])) { *reason = "missing required field"; return false; }

  const char* id = valueOf(json, "event_id");
  if (id[0] != '"' || id[1] == '"') { *reason = "event_id empty or not a string"; return false; }

  const char* t = valueOf(json, "event_type");
  if (strncmp(t, "\"accident\"", 10) != 0 && strncmp(t, "\"sos\"", 5) != 0) { *reason = "unknown event_type"; return false; }

  const char* ts = valueOf(json, "timestamp_utc");
  if (iso) { if (ts[0] != '"' && ts[0] != 'n') { *reason = "timestamp_utc must be an ISO string or null"; return false; } }
  else     { if (!(ts[0] >= '0' && ts[0] <= '9') && ts[0] != 'n') { *reason = "timestamp_utc must be an integer or null"; return false; } }

  const char* la = valueOf(json, "latitude");
  const char* lo = valueOf(json, "longitude");
  bool laNull = (la[0] == 'n'), loNull = (lo[0] == 'n');
  if (laNull != loNull) { *reason = "latitude and longitude must both be present or both null"; return false; }
  if (!laNull) {
    double lat = atof(la), lon = atof(lo);
    if (lat < -90.0 || lat > 90.0) { *reason = "latitude out of range"; return false; }
    if (lon < -180.0 || lon > 180.0) { *reason = "longitude out of range"; return false; }
  }
  const char* q = valueOf(json, "gnss_fix_quality");
  if (!(q[0] >= '0' && q[0] <= '9')) { *reason = "gnss_fix_quality must be an integer"; return false; }
  return true;
}

SendResult MockTransport::send(const char* eventId, const char* payload, uint32_t) {
  nAttempts++;
  if (mode == OFFLINE) return SendResult::FAILED_OFFLINE;
  if (mode == FAIL_N_THEN_ACK && failRemaining > 0) { failRemaining--; return SendResult::FAILED_OFFLINE; }

  // The backend receives the event.
  nReceived++;
  strncpy(lastPayloadBuf, payload, sizeof lastPayloadBuf - 1);
  const char* why = nullptr;
  if (!validate(payload, &why)) { nRejected++; lastReason = why; return SendResult::REJECTED; }

  bool known = false;
  for (int i = 0; i < nUnique; i++) if (strcmp(uniqueIds[i], eventId) == 0) { known = true; break; }
  if (known) nDuplicates++;                           // same event_id: recognised, not stored twice
  else if (nUnique < 16) strncpy(uniqueIds[nUnique++], eventId, EVENT_ID_MAX - 1);

  if (mode == ACK_LOST_ONCE && lostAckPending) { lostAckPending = false; return SendResult::ACK_TIMEOUT; }
  return SendResult::ACKED;
}

// ---------------- EventSender ----------------
void EventSender::begin(EventQueue* q, ITransport* t) {
  queue = q; transport = t; nSends = nAcks = nFails = 0;
}

int EventSender::tick(uint32_t nowMs, bool networkUp) {
  if (!networkUp) return 0;                           // offline: events stay queued
  QueueEntry* e = queue->nextDue(nowMs);
  if (!e) return 0;
  nSends++;
  SendResult r = transport->send(e->eventId, e->payload, nowMs);
  switch (r) {
    case SendResult::ACKED:   nAcks++; queue->markAcked(e->eventId); break;   // e is cleared here
    case SendResult::REJECTED: queue->markRejected(e); break;                 // not retried
    default:                  nFails++; queue->markFailed(e, nowMs); break;   // retry later, same payload
  }
  return 1;
}
