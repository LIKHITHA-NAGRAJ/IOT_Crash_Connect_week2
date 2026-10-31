#include <Arduino.h>
#include <stdio.h>
#include "App/app.h"
#include "Config/config.h"
#include "Sim/mock_at_port.h"
#include "Sim/sim_timeline.h"
#include "Storage/kv_store.h"

unsigned long fake_ms = 0;
SerialClass Serial;

int main() {
  static MockAtPort port;
  setvbuf(stdout, 0, _IOLBF, 0);
  sim_begin(0, &port);
  kv_begin("crashconn");
  config_provision("DEVICE-001");
  app_setup(0, &port, true);
  while (sim_step((uint32_t)fake_ms, false)) fake_ms += 20;
  sim_print_summary();
  return 0;
}
