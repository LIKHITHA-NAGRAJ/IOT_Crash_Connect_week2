#pragma once
#include <stdint.h>
#include "mock_at_port.h"

void sim_begin(uint32_t now_ms, MockAtPort *port);
bool sim_step(uint32_t now_ms, bool extra_sos);
void sim_print_summary();
