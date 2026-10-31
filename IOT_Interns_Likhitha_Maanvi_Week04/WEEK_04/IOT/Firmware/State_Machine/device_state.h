#pragma once
#include <stdint.h>
#include "../Common/cc_types.h"

struct StateInputs {
  bool self_test_pass;
  bool gps_fix;
  bool network_up;
  bool noncritical_fault;
  bool critical_fault;
  EventKind event;
  uint32_t time_in_state_ms;
};

DeviceState state_next(DeviceState current, const StateInputs &in, const char **reason);
void state_init(uint32_t now_ms);
DeviceState state_current();
bool state_update(StateInputs in, uint32_t now_ms, DeviceState *previous, const char **reason);
