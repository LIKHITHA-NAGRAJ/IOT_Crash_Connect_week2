#pragma once

#include <stdbool.h>
#include <stdint.h>

// LTE modem bring-up state machine for the SIMCom A7670C.
// *** MOCK ONLY: every response below comes from a software mock, NOT from a real modem. ***
// A state such as READY_SIMULATED does NOT mean real network registration, a real data
// session, or any real transmission. Real UART transport to the A7670C is NOT IMPLEMENTED.
//
// This module is kept separate from GNSS and telemetry on purpose.
// The data-transport step (TCP/MQTT versus PPP) is PENDING Technical Lead confirmation.

namespace Modem {

  enum State {
    NOT_STARTED,
    POWER_UP,
    AT_CHECK,
    SIM_CHECK,
    REGISTERING,
    SIGNAL_CHECK,
    APN_CONFIG,
    DATA_SESSION,
    READY_SIMULATED,          // terminal: mock data session "open" (NOT a real network)
    FAILED_NO_RESPONSE,       // terminal failures from here down
    FAILED_NO_SIM,
    FAILED_REG_DENIED,
    FAILED_REG_TIMEOUT,
    FAILED_APN_PENDING,
    FAILED_DATA_SESSION
  };

  // Which mock behaviour to play (for software tests only)
  enum MockScenario {
    MOCK_ALL_OK,
    MOCK_NO_RESPONSE,
    MOCK_NO_SIM,
    MOCK_REG_DENIED,
    MOCK_REG_NEVER,           // keeps "searching" until the timeout
    MOCK_WEAK_SIGNAL,
    MOCK_DATA_FAIL
  };

  struct Status {
    State   state;
    bool    simReady;
    bool    registered;
    int     rssi;             // AT+CSQ value 0-31, 99 = unknown; -1 = not read yet
    bool    signalWeak;
    bool    dataSession;      // mock data session flag
    uint8_t attempts;         // attempts in the current state
    bool    simulated;        // always true in this version
  };

  void begin(const char* apn, MockScenario scenario);   // apn "PENDING" or null -> FAILED_APN_PENDING
  void powerOn();                                       // starts the sequence (mock power-up)
  void step();                                          // call repeatedly; non-blocking
  Status getStatus();
  bool isTerminal();
  bool isFailed();
  const char* stateName(State s);
}
