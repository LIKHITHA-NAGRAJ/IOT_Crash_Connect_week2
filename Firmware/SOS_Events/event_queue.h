#pragma once

#include <stddef.h>
#include <stdint.h>

// Offline event queue that MODELS an intended flash-backed queue.
// SIMULATED STORAGE: the "flash" is a RAM copy. Real flash behaviour (wear, erase blocks,
// power loss during a write, file system) is NOT modelled and NOT validated.
// Events stay in the queue until acknowledged. The same event ID and the same payload
// bytes are used for every retry. Retries use exponential backoff with a cap.

const int EVENT_QUEUE_CAPACITY = 6;      // PLACEHOLDER
const int EVENT_PAYLOAD_MAX = 480;
const int EVENT_ID_MAX = 40;

enum class EntryState { EMPTY, PENDING, REJECTED };   // REJECTED = backend refused the payload; kept, not retried

struct QueueEntry {
  EntryState state;
  char     eventId[EVENT_ID_MAX];
  char     payload[EVENT_PAYLOAD_MAX];
  uint16_t attempts;
  uint32_t nextAttemptMs;
  uint32_t createdMs;
};

struct QueueStats {
  uint32_t pushed, acked, rejected, dropped;
  uint32_t flashWrites;            // number of simulated flash writes (a wear indicator)
};

class EventQueue {
public:
  // Restores from the simulated flash image (if any) and increments the persisted boot counter.
  void begin(uint32_t retryBaseMs, uint32_t retryMaxMs);
  // Test helper: erase the simulated flash image (as a factory reset would).
  static void eraseFlashImage();
  // Test helper: lose RAM and restore from the simulated flash image (a reboot).
  void simulateReboot();

  // Creates a unique ID like "<deviceId>-B0001-000012" and the matching sequence number.
  // The boot counter and the next sequence are persisted so IDs are never reused after a reboot.
  void allocateId(const char* deviceId, char* idOut, size_t idSize, uint32_t& sequenceOut);

  // Adds an event. If the queue is full the OLDEST pending entry is dropped (policy PENDING CONFIRMATION).
  bool push(const char* eventId, const char* payload, uint32_t nowMs);
  // Next pending entry whose retry time has arrived (oldest first), or nullptr.
  QueueEntry* nextDue(uint32_t nowMs);
  void markAcked(const char* eventId);
  void markFailed(QueueEntry* e, uint32_t nowMs);   // schedules the next attempt with backoff
  void markRejected(QueueEntry* e);

  int pending() const;
  int rejectedCount() const;
  const QueueEntry* find(const char* eventId) const;
  QueueStats stats() const;
  uint32_t bootCounter() const;

private:
  QueueEntry entries[EVENT_QUEUE_CAPACITY];
  uint32_t baseMs, maxMs;
  QueueStats st;
  uint32_t boot;
  uint32_t nextSeq;
  void saveToFlash();
  void loadFromFlash();
};
