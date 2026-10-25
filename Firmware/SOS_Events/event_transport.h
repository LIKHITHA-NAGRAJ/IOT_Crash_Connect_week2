#pragma once

#include <stdint.h>
#include "event_queue.h"

// Event transmission and acknowledgement logic.
// *** The transport and the "backend" below are SOFTWARE MOCKS. ***
// An ACK here is NOT a real backend response, and nothing is transmitted over a real modem
// or network. The mock backend validation reflects OUR ASSUMPTIONS about the contract, not Full Stack's real rules.

enum class SendResult { ACKED, FAILED_OFFLINE, ACK_TIMEOUT, REJECTED };

class ITransport {
public:
  virtual ~ITransport() {}
  virtual SendResult send(const char* eventId, const char* payload, uint32_t nowMs) = 0;
};

// Mock transport with a mock backend (SIMULATED).
class MockTransport : public ITransport {
public:
  enum Mode {
    ALWAYS_ACK,        // every send is received and acknowledged
    OFFLINE,           // nothing reaches the backend
    FAIL_N_THEN_ACK,   // first N sends fail (offline), then everything is acknowledged
    ACK_LOST_ONCE      // the backend receives the event but the first ACK is lost (timeout)
  };

  void begin(Mode m, int n = 0, bool isoTimestamp = false);
  void setMode(Mode m, int n = 0);
  SendResult send(const char* eventId, const char* payload, uint32_t nowMs) override;

  // Mock backend state ("dashboard" = the list of unique accepted events)
  int  receptions() const { return nReceived; }
  int  uniqueEvents() const { return nUnique; }
  int  duplicates() const { return nDuplicates; }
  int  rejectedCount() const { return nRejected; }
  int  sendAttempts() const { return nAttempts; }
  const char* uniqueId(int i) const { return (i >= 0 && i < nUnique) ? uniqueIds[i] : ""; }
  const char* lastPayload() const { return lastPayloadBuf; }
  const char* lastRejectReason() const { return lastReason; }

  // The mock validation rules (also usable on their own).
  bool validate(const char* json, const char** reason) const;

private:
  Mode mode;
  int  failRemaining;
  bool lostAckPending;
  bool iso;
  int  nAttempts, nReceived, nUnique, nDuplicates, nRejected;
  char uniqueIds[16][EVENT_ID_MAX];
  char lastPayloadBuf[EVENT_PAYLOAD_MAX];
  const char* lastReason;
};

// Sends queued events one per call, with retry and acknowledgement handling.
class EventSender {
public:
  void begin(EventQueue* q, ITransport* t);
  // Returns the number of send attempts made in this call (0 or 1).
  int tick(uint32_t nowMs, bool networkUp);
  uint32_t sends() const { return nSends; }
  uint32_t acks() const { return nAcks; }
  uint32_t failures() const { return nFails; }
private:
  EventQueue* queue;
  ITransport* transport;
  uint32_t nSends, nAcks, nFails;
};
