#include <Arduino.h>
#include "cc_app.h"
#include "cc_config.h"
#include "cc_kv_store.h"
#include "cc_mock_at_port.h"
#include "cc_sim_timeline.h"

static const int SOS_BUTTON_PIN = 10;

static MockAtPort port;
static bool finished = false;

bool sketch_finished() {
  return finished;
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  pinMode(SOS_BUTTON_PIN, INPUT_PULLUP);
  sim_begin(millis(), &port);
  kv_begin("crashconn");
  config_provision("DEVICE-001");
  app_setup(millis(), &port, true);
}

void loop() {
  if (finished) {
    delay(1000);
    return;
  }
  bool button = digitalRead(SOS_BUTTON_PIN) == LOW;
  if (!sim_step(millis(), button)) {
    sim_print_summary();
    finished = true;
    return;
  }
  delay(1);
}
