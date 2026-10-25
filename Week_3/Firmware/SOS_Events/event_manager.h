#pragma once

#include <stdint.h>
#include "accident_detector.h"
#include "event_builder.h"
#include "event_queue.h"
#include "event_transport.h"
#include "sos_button.h"

// Creates SOS and accident events, adds the device time and the GPS location (with its age),
// queues them, and sends them through the (MOCK) transport.
// Events are created and queued in ANY device state, including start-up, so none is lost.
// Everything is SIMULATED: no physical button, modem, flash or backend is involved.

namespace Events {
  // A location candidate used by the location rule (kept simple so it can be tested without waiting).
  struct LocInput { bool valid; double lat, lon; uint32_t ageS; uint8_t quality; };

  // Rule: a CURRENT valid fix is used with its age. Otherwise the LAST valid fix is used only if it is
  // no older than maxAgeS (age is reported, quality stays 0). Otherwise the location is null.
  void applyLocationRule(EventData& e, const LocInput& current, const LocInput& last, uint32_t maxAgeS);

  void begin(ITransport* transport);                       // restores the queue from simulated flash
  void setSimulatedClock(uint32_t epochAtNow, uint32_t nowMs);   // SIMULATED time (recorded GNSS time is not live)
  void clearSimulatedClock();

  bool createSos(uint32_t nowMs);
  bool createAccident(const AccidentResult& r, uint32_t nowMs);
  SosUpdate feedButton(bool rawPressed, uint32_t nowMs);   // creates an SOS event on a completed long press
  int  tick(uint32_t nowMs, bool networkUp);               // one send attempt at most

  EventQueue& queue();
  EventSender& sender();
  const char* lastEventId();
}
