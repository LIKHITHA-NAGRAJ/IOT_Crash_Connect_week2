#pragma once
#include <stdint.h>
#include "../Modem/modem.h"

struct AppStats {
  uint32_t telemetry_built;
  uint32_t status_built;
  uint32_t accident_events;
  uint32_t sos_events;
  uint32_t imu_invalid;
  uint32_t resets;
};

void app_setup(uint32_t now_ms, AtPort *port, bool simulated_clock);
void app_loop(uint32_t now_ms);
void app_sim_set_sos(bool pressed);
void app_sim_hang(bool hang);
bool app_sim_watchdog_check(uint32_t now_ms);
const AppStats &app_stats();
