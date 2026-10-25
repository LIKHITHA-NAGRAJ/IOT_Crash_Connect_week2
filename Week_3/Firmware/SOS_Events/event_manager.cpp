#include "event_manager.h"
#include "config.h"
#include "gnss.h"
#include <Arduino.h>
#include <string.h>

namespace Events {

  static EventQueue  q;
  static EventSender snd;
  static SosButton   button;
  static char        lastId[EVENT_ID_MAX];
  static bool        simClock = false;
  static uint32_t    simEpoch = 0, simAtMs = 0;

  EventQueue& queue() { return q; }
  EventSender& sender() { return snd; }
  const char* lastEventId() { return lastId; }

  void begin(ITransport* transport) {
    q.begin(CONFIG.events.retryBaseMs, CONFIG.events.retryMaxMs);
    snd.begin(&q, transport);
    button.begin(CONFIG.sos);
    lastId[0] = '\0';
  }

  void setSimulatedClock(uint32_t epoch, uint32_t nowMs) { simClock = true; simEpoch = epoch; simAtMs = nowMs; }
  void clearSimulatedClock() { simClock = false; }

  void applyLocationRule(EventData& e, const LocInput& cur, const LocInput& last, uint32_t maxAgeS) {
    e.locationValid = false;
    e.locationAgeValid = false;
    e.fixQuality = 0;
    if (cur.valid) {
      e.locationValid = true; e.lat = cur.lat; e.lon = cur.lon;
      e.locationAgeValid = true; e.locationAgeS = cur.ageS;
      e.fixQuality = cur.quality;
    } else if (last.valid && last.ageS <= maxAgeS) {
      e.locationValid = true; e.lat = last.lat; e.lon = last.lon;   // last known, NOT current: age says how old
      e.locationAgeValid = true; e.locationAgeS = last.ageS;
      e.fixQuality = 0;                                             // there is no current fix
    }
  }

  // Device time. A SIMULATED clock is used in tests. Recorded GNSS time is never live, so without
  // a simulated clock the timestamp is unavailable (null) unless a live GNSS time exists.
  static void fillTime(EventData& e, uint32_t nowMs) {
    e.timeValid = false;
    if (simClock) {
      e.timeValid = true;
      e.epochUtc = simEpoch + (nowMs - simAtMs) / 1000;
      return;
    }
    GNSS::Fix f;
    GNSS::getFix(f);
    if (f.valid && f.utcValid && f.timeIsLive) {
      e.timeValid = true;
      e.epochUtc = f.epochUtc + f.ageMs / 1000;
    }
  }

  static void fillLocation(EventData& e) {
    GNSS::Fix f;
    GNSS::getFix(f);
    LocInput cur = { f.valid, f.lat, f.lon, f.ageMs / 1000, f.fixQuality };
    GNSS::LastValid lv;
    bool haveLast = GNSS::getLastValid(lv) && lv.have;
    LocInput last = { haveLast, lv.lat, lv.lon, lv.ageMs / 1000, 0 };
    applyLocationRule(e, cur, last, CONFIG.events.maxLastKnownAgeS);
  }

  static bool createEvent(EventData& e, uint32_t nowMs) {
    EventFormat fmt = { CONFIG.events.schemaVersion, CONFIG.events.isoTimestamp, CONFIG.simulateHardware };
    uint32_t seq = 0;
    q.allocateId(CONFIG.deviceId, e.eventId, sizeof e.eventId, seq);
    e.sequence = seq;
    fillTime(e, nowMs);
    fillLocation(e);
    char json[EVENT_PAYLOAD_MAX];
    if (buildEventJson(json, sizeof json, CONFIG.deviceId, e, fmt) == 0) {
      Serial.println("[EVENT] payload too large, event not created");
      return false;
    }
    bool ok = q.push(e.eventId, json, nowMs);
    if (ok) {
      strncpy(lastId, e.eventId, sizeof lastId - 1);
      lastId[sizeof lastId - 1] = '\0';
      Serial.print("[EVENT] queued (SIMULATED transmission) id=");
      Serial.println(e.eventId);
    }
    return ok;
  }

  bool createSos(uint32_t nowMs) {
    EventData e = {};
    e.kind = EventKind::SOS;
    return createEvent(e, nowMs);
  }

  bool createAccident(const AccidentResult& r, uint32_t nowMs) {
    EventData e = {};
    e.kind = EventKind::ACCIDENT;
    e.accidentType = (r.orientationChanged || r.gyroSpike) ? "rollover_suspected" : "impact";
    e.confirmationState = r.lowConfidence ? "confirmed_low_confidence" : "confirmed";
    return createEvent(e, nowMs);
  }

  SosUpdate feedButton(bool rawPressed, uint32_t nowMs) {
    SosUpdate u = button.update(rawPressed, nowMs);
    if (u.feedback == SosFeedback::TRIGGERED || u.feedback == SosFeedback::RELEASED_EARLY ||
        u.feedback == SosFeedback::IGNORED_COOLDOWN || u.feedback == SosFeedback::PRESS_STARTED) {
      Serial.print("[SOS] feedback: ");
      Serial.println(sosFeedbackName(u.feedback));       // stands in for LED / buzzer feedback
    }
    if (u.triggered) createSos(nowMs);
    return u;
  }

  int tick(uint32_t nowMs, bool networkUp) { return snd.tick(nowMs, networkUp); }
}
