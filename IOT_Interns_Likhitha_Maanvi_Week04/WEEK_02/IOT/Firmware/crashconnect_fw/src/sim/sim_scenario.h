#pragma once
#include <stdint.h>

void sim_init(uint32_t now_ms);
void sim_update(uint32_t now_ms);
bool sim_is_done();
