#include "comms.h"
#include <Arduino.h>

namespace Comms {
  // No physical A7670C modem or cellular network is connected.
  static bool connected = false;

  bool begin() {
    connected = false;
    Serial.println("[COMMS] SIMULATION STUB initialized");
    Serial.println("[COMMS] Physical A7670C modem: NOT CONNECTED");
    return true; // Stub setup succeeded; modem is not tested.
  }

  bool connectNetwork() {
    connected = false;
    Serial.println("[COMMS] Network connection: NOT ATTEMPTED");
    Serial.println("[COMMS] Cellular connectivity is pending hardware integration.");
    return false;
  }

  bool sendData(const char* payload) {
    (void)payload;
    Serial.println("[COMMS] Telemetry NOT SENT - modem unavailable");
    return false;
  }

  bool isConnected() {
    return connected;
  }
}