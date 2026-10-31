#pragma once
#include <stdint.h>

const uint32_t FAULT_SELF_TEST    = 1u << 0;
const uint32_t FAULT_CONFIG       = 1u << 1;
const uint32_t FAULT_GNSS         = 1u << 2;
const uint32_t FAULT_IMU          = 1u << 3;
const uint32_t FAULT_COMMS        = 1u << 4;
const uint32_t FAULT_SIM_CRITICAL = 1u << 5;

const uint32_t FAULT_CRITICAL_MASK = FAULT_SELF_TEST | FAULT_CONFIG | FAULT_SIM_CRITICAL;

void fault_set(uint32_t bits);
void fault_clear(uint32_t bits);
void fault_clear_all();
uint32_t fault_flags();
bool fault_is_critical();
bool fault_is_noncritical();
