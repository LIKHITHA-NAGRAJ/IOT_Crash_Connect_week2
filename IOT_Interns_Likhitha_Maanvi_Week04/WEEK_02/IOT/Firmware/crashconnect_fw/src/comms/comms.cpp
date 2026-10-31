#include <Arduino.h>
#include "comms.h"
#include "../config/config.h"

namespace {

bool initialised = false;
bool connected = false;
bool simNetworkUp = false;
bool hwWarned = false;
uint32_t sentCount = 0;
uint32_t failedCount = 0;

}

bool comms_init() {
  connected = false;
  initialised = true;
  return true;
}

void comms_update(uint32_t now_ms) {
  (void)now_ms;
  if (config().use_simulated_inputs) {
    connected = simNetworkUp;
    return;
  }
  if (!hwWarned) {
    Serial.println("[COMMS][STUB] A7670C AT-command link not implemented (UART pins, power-on sequence, MQTT/API choice: PENDING CONFIRMATION)");
    hwWarned = true;
  }
  connected = false;
}

bool comms_is_connected() {
  return connected;
}

bool comms_send(const char *topic, const char *payload, size_t len) {
  (void)payload;
  if (!connected) {
    failedCount++;
    Serial.printf("[COMMS][STUB] NOT SENT, network down (%u bytes)\n", (unsigned)len);
    return false;
  }
  sentCount++;
  Serial.printf("[COMMS][STUB] would send %u bytes, topic/path: %s, transport: %s\n",
                (unsigned)len, topic, config().comms.transport);
  return true;
}

void comms_sim_set_network(bool up) {
  simNetworkUp = up;
}

uint32_t comms_sent_count() {
  return sentCount;
}

uint32_t comms_failed_count() {
  return failedCount;
}

bool comms_module_ok() {
  return initialised;
}
