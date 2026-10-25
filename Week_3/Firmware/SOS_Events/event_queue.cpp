#include "event_queue.h"
#include <stdio.h>
#include <string.h>

// ---- SIMULATED flash image (a RAM copy that survives begin() and simulateReboot()) ----
namespace {
  struct FlashImage {
    bool        valid;
    QueueEntry  entries[EVENT_QUEUE_CAPACITY];
    uint32_t    boot;
    uint32_t    nextSeq;
    uint32_t    writes;
  };
  FlashImage flashImage;
}

void EventQueue::eraseFlashImage() { memset(&flashImage, 0, sizeof flashImage); }

void EventQueue::saveToFlash() {
  memcpy(flashImage.entries, entries, sizeof entries);
  flashImage.boot = boot;
  flashImage.nextSeq = nextSeq;
  flashImage.valid = true;
  flashImage.writes++;
  st.flashWrites = flashImage.writes;
}

void EventQueue::loadFromFlash() {
  if (flashImage.valid) {
    memcpy(entries, flashImage.entries, sizeof entries);
    boot = flashImage.boot;
    nextSeq = flashImage.nextSeq;
  } else {
    memset(entries, 0, sizeof entries);
    boot = 0;
    nextSeq = 1;
  }
}

void EventQueue::begin(uint32_t retryBaseMs, uint32_t retryMaxMs) {
  baseMs = retryBaseMs;
  maxMs = retryMaxMs;
  uint32_t keepWrites = flashImage.writes;
  memset(&st, 0, sizeof st);
  loadFromFlash();
  boot++;                                  // every start is a new boot
  st.flashWrites = keepWrites;
  saveToFlash();
}

void EventQueue::simulateReboot() {
  uint32_t b = baseMs, m = maxMs;
  begin(b, m);
}

void EventQueue::allocateId(const char* deviceId, char* idOut, size_t idSize, uint32_t& seqOut) {
  seqOut = nextSeq++;
  snprintf(idOut, idSize, "%s-B%04lu-%06lu", deviceId, (unsigned long)boot, (unsigned long)seqOut);
  saveToFlash();                           // persist the counter before the ID is used
}

bool EventQueue::push(const char* eventId, const char* payload, uint32_t nowMs) {
  if (strlen(payload) >= EVENT_PAYLOAD_MAX || strlen(eventId) >= EVENT_ID_MAX) return false;
  int slot = -1;
  for (int i = 0; i < EVENT_QUEUE_CAPACITY; i++) if (entries[i].state == EntryState::EMPTY) { slot = i; break; }
  if (slot < 0) {                          // full: drop the oldest PENDING entry
    uint32_t oldest = 0xFFFFFFFFu;
    for (int i = 0; i < EVENT_QUEUE_CAPACITY; i++)
      if (entries[i].state == EntryState::PENDING && entries[i].createdMs <= oldest) { oldest = entries[i].createdMs; slot = i; }
    if (slot < 0) return false;            // nothing droppable
    st.dropped++;
  }
  QueueEntry& e = entries[slot];
  memset(&e, 0, sizeof e);
  e.state = EntryState::PENDING;
  strncpy(e.eventId, eventId, sizeof e.eventId - 1);
  strncpy(e.payload, payload, sizeof e.payload - 1);
  e.attempts = 0;
  e.nextAttemptMs = nowMs;
  e.createdMs = nowMs;
  st.pushed++;
  saveToFlash();
  return true;
}

QueueEntry* EventQueue::nextDue(uint32_t nowMs) {
  QueueEntry* best = nullptr;
  for (int i = 0; i < EVENT_QUEUE_CAPACITY; i++) {
    QueueEntry& e = entries[i];
    if (e.state != EntryState::PENDING || e.nextAttemptMs > nowMs) continue;
    if (!best || e.createdMs < best->createdMs) best = &e;
  }
  return best;
}

void EventQueue::markAcked(const char* eventId) {
  for (int i = 0; i < EVENT_QUEUE_CAPACITY; i++)
    if (entries[i].state != EntryState::EMPTY && strcmp(entries[i].eventId, eventId) == 0) {
      memset(&entries[i], 0, sizeof entries[i]);
      st.acked++;
      saveToFlash();
      return;
    }
}

void EventQueue::markFailed(QueueEntry* e, uint32_t nowMs) {
  e->attempts++;
  uint32_t delay = baseMs;
  for (uint16_t i = 1; i < e->attempts && delay < maxMs; i++) delay *= 2;
  if (delay > maxMs) delay = maxMs;
  e->nextAttemptMs = nowMs + delay;
  saveToFlash();
}

void EventQueue::markRejected(QueueEntry* e) {
  e->attempts++;
  e->state = EntryState::REJECTED;
  st.rejected++;
  saveToFlash();
}

int EventQueue::pending() const {
  int n = 0;
  for (int i = 0; i < EVENT_QUEUE_CAPACITY; i++) if (entries[i].state == EntryState::PENDING) n++;
  return n;
}
int EventQueue::rejectedCount() const {
  int n = 0;
  for (int i = 0; i < EVENT_QUEUE_CAPACITY; i++) if (entries[i].state == EntryState::REJECTED) n++;
  return n;
}
const QueueEntry* EventQueue::find(const char* eventId) const {
  for (int i = 0; i < EVENT_QUEUE_CAPACITY; i++)
    if (entries[i].state != EntryState::EMPTY && strcmp(entries[i].eventId, eventId) == 0) return &entries[i];
  return nullptr;
}
QueueStats EventQueue::stats() const { return st; }
uint32_t EventQueue::bootCounter() const { return boot; }
