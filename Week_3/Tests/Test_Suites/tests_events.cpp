#include <Arduino.h>
#include <stdio.h>
#include <string.h>
#include "config.h"
#include "sos_button.h"
#include "event_builder.h"
#include "event_queue.h"
#include "event_transport.h"
#include "event_manager.h"
#include "device_integration.h"
#include "state_machine.h"
#include "gnss.h"
#include "sim_scenarios.h"
#include "tests.h"

// Manual SOS, accident events, retry, acknowledgement and offline-queue tests.
// *** EVERYTHING HERE IS SIMULATED: the button is a boolean signal, the flash is a RAM copy,
// *** the modem and backend are mocks, and time is the simulator's. An ACK is NOT a real backend reply.
// A pass shows the software logic works. It validates no physical button, modem, flash or backend.

static int passed = 0, failed = 0;
static void check(bool ok, const char* label) {
  Serial.print(ok ? "  PASS: " : "  FAIL: ");
  Serial.println(label);
  if (ok) passed++; else failed++;
}
static bool has(const char* s, const char* sub) { return strstr(s, sub) != nullptr; }

static void replaceOnce(char* buf, size_t size, const char* from, const char* to) {
  char* p = strstr(buf, from);
  if (!p) return;
  char tmp[EVENT_PAYLOAD_MAX];
  size_t head = (size_t)(p - buf);
  snprintf(tmp, sizeof tmp, "%.*s%s%s", (int)head, buf, to, p + strlen(from));
  strncpy(buf, tmp, size - 1);
  buf[size - 1] = '\0';
}

static const EventFormat FMT_EPOCH = { "1.0", false, true };
static const EventFormat FMT_ISO   = { "1.0", true,  true };

// ------------------------------------------------------------------ A: SOS button
struct BtnLog { int triggers; bool started, early, ignored; uint8_t maxProgress; uint32_t triggerAt; };

static void drive(SosButton& b, bool level, uint32_t from, uint32_t to, BtnLog& log) {
  for (uint32_t t = from; t < to; t += 10) {
    SosUpdate u = b.update(level, t);
    if (u.triggered) { log.triggers++; log.triggerAt = t; }
    if (u.feedback == SosFeedback::PRESS_STARTED) log.started = true;
    if (u.feedback == SosFeedback::RELEASED_EARLY) log.early = true;
    if (u.feedback == SosFeedback::IGNORED_COOLDOWN) log.ignored = true;
    if (u.feedback == SosFeedback::HOLDING && u.progressPct > log.maxProgress) log.maxProgress = u.progressPct;
  }
}

static void testButton() {
  Serial.println("\nA. Manual SOS long press (simulated signal)");
  SosButton b;
  b.begin(CONFIG.sos);
  BtnLog log = {};

  // A1: contact bounce shorter than the debounce time
  for (uint32_t t = 0; t < 100; t += 5) b.update(((t / 5) % 2) == 0, t);
  for (uint32_t t = 100; t < 400; t += 10) { SosUpdate u = b.update(false, t); if (u.feedback == SosFeedback::PRESS_STARTED) log.started = true; }
  check(!log.started && !b.pressed(), "A1 contact bounce is ignored (no press registered)");

  // A2: short press
  log = {};
  drive(b, true, 1000, 1500, log);
  drive(b, false, 1500, 1800, log);
  check(log.started && log.early && log.triggers == 0, "A2 short press: feedback RELEASED_EARLY, no SOS");

  // A3: long press
  log = {};
  drive(b, true, 3000, 6000, log);
  drive(b, false, 6000, 6300, log);
  check(log.triggers == 1, "A3 long press triggers exactly once (not again while still held)");
  check(log.triggerAt >= 3000 + CONFIG.sos.longPressMs && log.triggerAt <= 3000 + CONFIG.sos.longPressMs + 40,
        "A3 trigger happens at the configured hold time");
  check(log.maxProgress >= 80, "A3 progress feedback was reported while holding");

  // A4: pressing again inside the cooldown
  log = {};
  drive(b, true, 6500, 9500, log);
  drive(b, false, 9500, 9800, log);
  check(log.ignored && log.triggers == 0, "A4 press during cooldown is ignored");

  // A5: after the cooldown a new long press works
  log = {};
  drive(b, true, 12000, 15000, log);
  drive(b, false, 15000, 15300, log);
  check(log.triggers == 1, "A5 long press after the cooldown triggers again");
}

// ------------------------------------------------------------------ B: event payloads
static EventData sample(EventKind k) {
  EventData e = {};
  strcpy(e.eventId, "CC-IOT-01-B0001-000001");
  e.sequence = 1;
  e.kind = k;
  e.timeValid = true;
  e.epochUtc = 1792582500UL;
  e.locationValid = true; e.lat = 48.1173; e.lon = 11.516667;
  e.locationAgeValid = true; e.locationAgeS = 0;
  e.fixQuality = 1;
  return e;
}

static void testBuilder() {
  Serial.println("\nB. Event payloads (recorded sample coordinates, simulated time)");
  char j[EVENT_PAYLOAD_MAX];
  const char* why = nullptr;
  MockTransport mt; mt.begin(MockTransport::ALWAYS_ACK, 0, false);

  EventData e = sample(EventKind::SOS);
  check(buildEventJson(j, sizeof j, "CC-IOT-01", e, FMT_EPOCH) > 0, "B1 SOS payload built");
  Serial.print("  "); Serial.println(j);
  check(has(j, "\"event_id\":\"CC-IOT-01-B0001-000001\"") && has(j, "\"event_type\":\"sos\"") &&
        has(j, "\"timestamp_utc\":1792582500") && has(j, "\"location_age_s\":0") &&
        has(j, "\"gnss_fix_quality\":1") && has(j, "\"snapshot\":null") && has(j, "\"simulated\":true"),
        "B1 has event ID, type, integer timestamp, location age, fix quality, snapshot null, simulated flag");
  check(mt.validate(j, &why), "B1 accepted by the mock backend rules");

  e = sample(EventKind::SOS);
  e.timeValid = false; e.locationValid = false; e.locationAgeValid = false; e.fixQuality = 0;
  buildEventJson(j, sizeof j, "CC-IOT-01", e, FMT_EPOCH);
  check(has(j, "\"timestamp_utc\":null") && has(j, "\"latitude\":null,\"longitude\":null") &&
        has(j, "\"location_age_s\":null") && has(j, "\"gnss_fix_quality\":0"),
        "B2 no location and no time: everything null, no invented coordinates");
  check(mt.validate(j, &why), "B2 null location accepted by the mock backend rules");

  e = sample(EventKind::SOS);
  e.locationAgeS = 12; e.fixQuality = 0;
  buildEventJson(j, sizeof j, "CC-IOT-01", e, FMT_EPOCH);
  check(has(j, "\"latitude\":48.117300") && has(j, "\"location_age_s\":12") && has(j, "\"gnss_fix_quality\":0"),
        "B3 last-known location is sent with its age, fix quality 0 (no current fix)");

  e = sample(EventKind::ACCIDENT); e.accidentType = "impact"; e.confirmationState = "confirmed";
  buildEventJson(j, sizeof j, "CC-IOT-01", e, FMT_EPOCH);
  check(has(j, "\"event_type\":\"accident\"") && has(j, "\"accident_type\":\"impact\"") && has(j, "\"confirmation_state\":\"confirmed\""),
        "B4 accident event has type, accident type and confirmation state");
  e.accidentType = "rollover_suspected"; e.confirmationState = "confirmed_low_confidence";
  buildEventJson(j, sizeof j, "CC-IOT-01", e, FMT_EPOCH);
  check(has(j, "rollover_suspected") && has(j, "confirmed_low_confidence"), "B4 rollover and low-confidence variants");

  e = sample(EventKind::ACCIDENT); e.epochUtc = 1792582200UL;
  buildEventJson(j, sizeof j, "CC-IOT-01", e, FMT_ISO);
  check(has(j, "\"timestamp_utc\":\"2026-10-21T11:30:00Z\""), "B5 ISO 8601 option produces the sample's text format");

  char small[60];
  check(buildEventJson(small, sizeof small, "CC-IOT-01", sample(EventKind::SOS), FMT_EPOCH) == 0 && small[0] == '\0',
        "B6 buffer too small: returns 0 and an empty string");

  // Location rule: current fix, last known within limit, last known too old, nothing.
  Events::LocInput none = { false, 0, 0, 0, 0 };
  Events::LocInput cur = { true, 10.0, 20.0, 1, 1 };
  Events::LocInput last = { true, 11.0, 21.0, 30, 0 };
  Events::LocInput oldLast = { true, 11.0, 21.0, 601, 0 };
  EventData r = {};
  Events::applyLocationRule(r, cur, last, 600);
  check(r.locationValid && r.lat == 10.0 && r.fixQuality == 1, "B7 current fix wins");
  r = {};
  Events::applyLocationRule(r, none, last, 600);
  check(r.locationValid && r.lat == 11.0 && r.locationAgeS == 30 && r.fixQuality == 0, "B7 last-known location used with its age, quality 0");
  r = {};
  Events::applyLocationRule(r, none, oldLast, 600);
  check(!r.locationValid && !r.locationAgeValid, "B7 last-known location older than the limit is reported as unavailable");
  r = {};
  Events::applyLocationRule(r, none, none, 600);
  check(!r.locationValid && r.fixQuality == 0, "B7 never had a fix: unavailable");

  // Invalid payloads must be rejected by the mock backend rules.
  char base[EVENT_PAYLOAD_MAX], bad[EVENT_PAYLOAD_MAX];
  buildEventJson(base, sizeof base, "CC-IOT-01", sample(EventKind::SOS), FMT_EPOCH);
  strcpy(bad, base); replaceOnce(bad, sizeof bad, "\"event_id\":", "\"event_idX\":");
  check(!mt.validate(bad, &why), "B8 missing event_id rejected");
  strcpy(bad, base); replaceOnce(bad, sizeof bad, "\"sos\"", "\"fire\"");
  check(!mt.validate(bad, &why), "B8 unknown event_type rejected");
  EventData far = sample(EventKind::SOS); far.lat = 95.0;
  buildEventJson(bad, sizeof bad, "CC-IOT-01", far, FMT_EPOCH);
  check(!mt.validate(bad, &why), "B8 latitude out of range rejected");
  strcpy(bad, base); replaceOnce(bad, sizeof bad, "\"timestamp_utc\":1792582500", "\"timestamp_utc\":\"2026-10-21T11:35:00Z\"");
  check(!mt.validate(bad, &why), "B8 ISO text rejected when an integer timestamp is expected");
  strcpy(bad, base); replaceOnce(bad, sizeof bad, "\"longitude\":11.516667", "\"longitude\":null");
  check(!mt.validate(bad, &why), "B8 latitude present but longitude null rejected");
}

// ------------------------------------------------------------------ C: queue, retry, acknowledgement
static void makeEvent(EventQueue& q, EventKind kind, uint32_t now, char* idOut, size_t idSize, char* payloadOut) {
  EventData e = {};
  uint32_t seq = 0;
  q.allocateId("CC-IOT-01", idOut, idSize, seq);
  strncpy(e.eventId, idOut, sizeof e.eventId - 1);
  e.sequence = seq; e.kind = kind;
  e.timeValid = true; e.epochUtc = 1792582500UL + now / 1000;
  e.locationValid = true; e.lat = 48.1173; e.lon = 11.516667;
  e.locationAgeValid = true; e.locationAgeS = 0; e.fixQuality = 1;
  e.accidentType = "impact"; e.confirmationState = "confirmed";
  buildEventJson(payloadOut, EVENT_PAYLOAD_MAX, "CC-IOT-01", e, FMT_EPOCH);
  q.push(idOut, payloadOut, now);
}

static void testQueue() {
  Serial.println("\nC. Queue, retry and acknowledgement (MOCK transport and backend, simulated flash)");
  EventQueue q; EventSender s; MockTransport mt;
  char id[EVENT_ID_MAX], id2[EVENT_ID_MAX], id3[EVENT_ID_MAX], pl[EVENT_PAYLOAD_MAX], pl0[EVENT_PAYLOAD_MAX];

  // C1: online, acknowledged at once
  EventQueue::eraseFlashImage();
  q.begin(CONFIG.events.retryBaseMs, CONFIG.events.retryMaxMs);
  mt.begin(MockTransport::ALWAYS_ACK); s.begin(&q, &mt);
  makeEvent(q, EventKind::SOS, 0, id, sizeof id, pl);
  s.tick(0, true);
  check(q.pending() == 0 && mt.uniqueEvents() == 1 && mt.sendAttempts() == 1 && s.acks() == 1, "C1 online: sent once, acknowledged, queue empty");

  // C2: offline, then connectivity returns
  EventQueue::eraseFlashImage();
  q.begin(CONFIG.events.retryBaseMs, CONFIG.events.retryMaxMs);
  mt.begin(MockTransport::ALWAYS_ACK); s.begin(&q, &mt);
  makeEvent(q, EventKind::SOS, 100, id, sizeof id, pl);
  makeEvent(q, EventKind::ACCIDENT, 200, id2, sizeof id2, pl);
  makeEvent(q, EventKind::SOS, 300, id3, sizeof id3, pl);
  int offlineSends = 0;
  for (uint32_t t = 1000; t < 6000; t += 500) offlineSends += s.tick(t, false);
  check(offlineSends == 0 && q.pending() == 3 && mt.sendAttempts() == 0, "C2 offline: nothing is sent, 3 events wait in the queue");
  for (uint32_t t = 6000; t < 6600; t += 100) s.tick(t, true);
  check(q.pending() == 0 && mt.uniqueEvents() == 3, "C2 connectivity back: all 3 events sent and acknowledged");
  check(strcmp(mt.uniqueId(0), id) == 0 && strcmp(mt.uniqueId(1), id2) == 0 && strcmp(mt.uniqueId(2), id3) == 0, "C2 events are sent oldest first");

  // C3: transmission fails twice, then works. Same ID and payload each time. Backoff doubles.
  EventQueue::eraseFlashImage();
  q.begin(CONFIG.events.retryBaseMs, CONFIG.events.retryMaxMs);
  mt.begin(MockTransport::FAIL_N_THEN_ACK, 2); s.begin(&q, &mt);
  makeEvent(q, EventKind::SOS, 0, id, sizeof id, pl0);
  s.tick(0, true);
  check(q.pending() == 1 && q.find(id)->attempts == 1 && q.find(id)->nextAttemptMs == CONFIG.events.retryBaseMs, "C3 first failure: retry scheduled after the base delay");
  check(s.tick(CONFIG.events.retryBaseMs - 1, true) == 0, "C3 no attempt before the retry time");
  s.tick(CONFIG.events.retryBaseMs, true);
  check(q.find(id)->attempts == 2 && q.find(id)->nextAttemptMs == CONFIG.events.retryBaseMs + 2 * CONFIG.events.retryBaseMs, "C3 second failure: delay doubled");
  check(strcmp(q.find(id)->payload, pl0) == 0, "C3 payload and event ID unchanged across retries");
  s.tick(q.find(id)->nextAttemptMs, true);
  check(q.pending() == 0 && mt.sendAttempts() == 3 && mt.uniqueEvents() == 1 && strcmp(mt.lastPayload(), pl0) == 0,
        "C3 third attempt acknowledged; backend got the identical payload");

  // C4: the backend receives it but the ACK is lost. The retry must not create a second event.
  EventQueue::eraseFlashImage();
  q.begin(CONFIG.events.retryBaseMs, CONFIG.events.retryMaxMs);
  mt.begin(MockTransport::ACK_LOST_ONCE); s.begin(&q, &mt);
  makeEvent(q, EventKind::SOS, 0, id, sizeof id, pl0);
  s.tick(0, true);
  check(q.pending() == 1 && mt.receptions() == 1, "C4 ACK lost: the event stays queued");
  s.tick(CONFIG.events.retryBaseMs, true);
  check(q.pending() == 0 && mt.receptions() == 2 && mt.uniqueEvents() == 1 && mt.duplicates() == 1 && strcmp(mt.lastPayload(), pl0) == 0,
        "C4 retry with the same ID: backend sees a duplicate (1 unique event), then ACK");

  // C5: the backend rejects the payload. It is not retried forever.
  EventQueue::eraseFlashImage();
  q.begin(CONFIG.events.retryBaseMs, CONFIG.events.retryMaxMs);
  mt.begin(MockTransport::ALWAYS_ACK); s.begin(&q, &mt);
  makeEvent(q, EventKind::SOS, 0, id, sizeof id, pl);
  QueueEntry* qe = q.nextDue(0);
  replaceOnce(qe->payload, sizeof qe->payload, "\"sos\"", "\"fire\"");   // corrupt it
  s.tick(0, true);
  int sendsBefore = mt.sendAttempts();
  for (uint32_t t = 1000; t < 20000; t += 1000) s.tick(t, true);
  check(q.rejectedCount() == 1 && q.pending() == 0 && mt.sendAttempts() == sendsBefore && mt.uniqueEvents() == 0,
        "C5 rejected payload: marked REJECTED, kept for inspection, not retried");

  // C6: reboot while offline. Events persist in the (simulated) flash and IDs are never reused.
  EventQueue::eraseFlashImage();
  q.begin(CONFIG.events.retryBaseMs, CONFIG.events.retryMaxMs);
  mt.begin(MockTransport::ALWAYS_ACK); s.begin(&q, &mt);
  makeEvent(q, EventKind::SOS, 0, id, sizeof id, pl0);
  makeEvent(q, EventKind::ACCIDENT, 100, id2, sizeof id2, pl);
  check(q.bootCounter() == 1, "C6 first boot is boot 1");
  q.simulateReboot();
  check(q.bootCounter() == 2 && q.pending() == 2 && q.find(id) && strcmp(q.find(id)->payload, pl0) == 0,
        "C6 after a reboot the events are restored unchanged and the boot counter increased");
  makeEvent(q, EventKind::SOS, 500, id3, sizeof id3, pl);
  check(strcmp(id3, id) != 0 && strcmp(id3, id2) != 0 && has(id3, "-B0002-"), "C6 a new event after the reboot gets a new, unique ID");
  for (uint32_t t = 1000; t < 1400; t += 100) s.tick(t, true);
  check(q.pending() == 0 && mt.uniqueEvents() == 3 && strcmp(mt.uniqueId(0), id) == 0, "C6 restored events are sent with their original IDs");
  Serial.print("  simulated flash writes so far: "); Serial.println((unsigned)q.stats().flashWrites);

  // C7: queue full while offline
  EventQueue::eraseFlashImage();
  q.begin(CONFIG.events.retryBaseMs, CONFIG.events.retryMaxMs);
  mt.begin(MockTransport::OFFLINE); s.begin(&q, &mt);
  char first[EVENT_ID_MAX] = "";
  for (int i = 0; i < EVENT_QUEUE_CAPACITY + 1; i++) {
    makeEvent(q, EventKind::SOS, (uint32_t)(i * 100), id, sizeof id, pl);
    if (i == 0) strcpy(first, id);
  }
  check(q.pending() == EVENT_QUEUE_CAPACITY && q.stats().dropped == 1 && q.find(first) == nullptr,
        "C7 queue full: oldest event dropped (policy PENDING CONFIRMATION), count reported");
}

// ------------------------------------------------------------------ D: integration with GPS, detector and state machine
struct FeedCtx { static void cb(const SimSample& s, void*) {
  if (s.speedState == 1) Integration::feedGpsSpeed(true, s.speedKmh);
  else if (s.speedState == 2) Integration::feedGpsSpeed(false, 0.0f);
  ImuInput in = { s.tMs, s.ax, s.ay, s.az, s.gx, s.gy, s.gz };
  Integration::feedImu(in);
} };

static void holdButton(uint32_t holdMs) {
  uint32_t t0 = millis();
  while (millis() - t0 < holdMs) { Integration::feedSosButton(true, millis()); delay(10); }
  for (int i = 0; i < 10; i++) { Integration::feedSosButton(false, millis()); delay(10); }
}

static void testIntegration() {
  Serial.println("\nD. SOS and accident events with GPS validity and the state machine (simulated)");
  const char* GGA = "$GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47";
  const char* RMC = "$GPRMC,123519,A,4807.038,N,01131.000,E,022.4,084.4,230394,003.1,W*6A";
  MockTransport mt;
  DeviceState::Inputs ok = { true, true, true, false, false, false };

  // D1: accident detected by the staged detector
  EventQueue::eraseFlashImage();
  mt.begin(MockTransport::ALWAYS_ACK);
  Events::begin(&mt);
  Events::setSimulatedClock(1792582200UL, millis());
  GNSS::begin(); GNSS::feedLine(GGA); GNSS::feedLine(RMC);
  DeviceState::begin();
  for (int i = 0; i < 4; i++) DeviceState::step(ok);
  Integration::accidentBegin(CONFIG.accident);
  generateScenario(9, FeedCtx::cb, nullptr);                       // simulated frontal impact
  const QueueEntry* ev = Events::queue().find(Events::lastEventId());
  check(ev != nullptr && Events::queue().pending() == 1, "D1 confirmed accident created and queued one event");
  if (ev) {
    check(has(ev->payload, "\"event_type\":\"accident\"") && has(ev->payload, "\"accident_type\":\"impact\"") &&
          has(ev->payload, "\"confirmation_state\":\"confirmed\""), "D1 accident type and confirmation state present");
    check(has(ev->payload, "\"latitude\":48.117300") && has(ev->payload, "\"gnss_fix_quality\":1") && has(ev->payload, "\"timestamp_utc\":1792582"),
          "D1 location from the current recorded fix, quality 1, simulated integer timestamp");
  }
  DeviceState::Inputs in = ok;
  Integration::applyEventLatch(in);
  DeviceState::step(in);
  check(DeviceState::get() == DeviceState::EVENT_ACTIVE, "D1 state machine: OPERATIONAL to EVENT_ACTIVE");
  Events::tick(millis(), true);
  check(Events::queue().pending() == 0 && mt.uniqueEvents() == 1, "D1 event sent and (mock) acknowledged");

  // D2: manual SOS with a valid fix
  EventQueue::eraseFlashImage();
  mt.begin(MockTransport::ALWAYS_ACK);
  Events::begin(&mt);
  Events::setSimulatedClock(1792582500UL, millis());
  GNSS::begin(); GNSS::feedLine(GGA); GNSS::feedLine(RMC);
  DeviceState::begin();
  for (int i = 0; i < 4; i++) DeviceState::step(ok);
  Integration::accidentBegin(CONFIG.accident);
  holdButton(CONFIG.sos.longPressMs + 300);
  ev = Events::queue().find(Events::lastEventId());
  // The recorded fix is not refreshed during the hold, so its age is a few seconds, not 0.
  check(ev != nullptr && has(ev->payload, "\"event_type\":\"sos\"") && has(ev->payload, "\"latitude\":48.117300") &&
        has(ev->payload, "\"gnss_fix_quality\":1") && !has(ev->payload, "\"location_age_s\":null"),
        "D2 long press created an SOS event with the current location and its age");
  in = ok; Integration::applyEventLatch(in); DeviceState::step(in);
  check(DeviceState::get() == DeviceState::EVENT_ACTIVE, "D2 state machine: SOS moves OPERATIONAL to EVENT_ACTIVE");
  Events::tick(millis(), true);
  check(Events::queue().pending() == 0 && mt.uniqueEvents() == 1, "D2 SOS sent and (mock) acknowledged");

  // D3: SOS pressed while the device is still starting (no GPS fix, no network)
  EventQueue::eraseFlashImage();
  mt.begin(MockTransport::ALWAYS_ACK);
  Events::begin(&mt);
  Events::setSimulatedClock(1792582500UL, millis());
  GNSS::begin();                                                   // no fix
  DeviceState::begin();
  DeviceState::step(ok); DeviceState::step(ok);                    // BOOT, SELF_TEST, now ACQUIRING_GPS
  Integration::accidentBegin(CONFIG.accident);
  holdButton(CONFIG.sos.longPressMs + 300);
  ev = Events::queue().find(Events::lastEventId());
  check(ev != nullptr && has(ev->payload, "\"latitude\":null") && has(ev->payload, "\"location_age_s\":null") && has(ev->payload, "\"gnss_fix_quality\":0"),
        "D3 no fix and never had one: location unavailable, no fake coordinates");
  DeviceState::Inputs starting = ok; starting.gpsFix = false; starting.networkUp = false;
  Integration::applyEventLatch(starting);
  DeviceState::step(starting);
  check(DeviceState::get() == DeviceState::ACQUIRING_GPS, "D3 start-up state is not disturbed");
  check(Events::tick(millis(), false) == 0 && Events::queue().pending() == 1, "D3 no network: the SOS stays queued");
  Events::tick(millis(), true);
  check(Events::queue().pending() == 0 && mt.uniqueEvents() == 1, "D3 network available: the queued SOS is sent");

  // D4: GPS lost after a good fix. The last valid location is sent with its age.
  EventQueue::eraseFlashImage();
  mt.begin(MockTransport::ALWAYS_ACK);
  Events::begin(&mt);
  Events::setSimulatedClock(1792582500UL, millis());
  GNSS::begin(); GNSS::feedLine(GGA); GNSS::feedLine(RMC);
  delay(CONFIG.timeouts.gnssStaleMs + 1500);                       // the fix goes stale
  Integration::accidentBegin(CONFIG.accident);
  holdButton(CONFIG.sos.longPressMs + 300);
  ev = Events::queue().find(Events::lastEventId());
  check(ev != nullptr && has(ev->payload, "\"latitude\":48.117300") && has(ev->payload, "\"gnss_fix_quality\":0") &&
        !has(ev->payload, "\"location_age_s\":null") && !has(ev->payload, "\"location_age_s\":0,"),
        "D4 GPS lost: last valid location sent with its age, fix quality 0");
  Events::clearSimulatedClock();
}

int runEventTests() {
  Serial.println("\n=== SOS, ACCIDENT EVENTS, RETRY AND OFFLINE QUEUE (SIMULATED: mock modem/backend, simulated flash) ===");
  testButton();
  testBuilder();
  testQueue();
  testIntegration();
  Serial.print("\nRESULT: "); Serial.print(passed); Serial.print(" passed, ");
  Serial.print(failed); Serial.println(" failed");
  Serial.println("NOTE: simulated button, flash, modem and backend. An ACK here is not a real backend reply.");
  return failed;
}
