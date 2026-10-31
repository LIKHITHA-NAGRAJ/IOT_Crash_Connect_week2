#pragma once
#include <stddef.h>
#include <stdint.h>

bool comms_init();
void comms_update(uint32_t now_ms);
bool comms_is_connected();
bool comms_send(const char *topic, const char *payload, size_t len);
void comms_sim_set_network(bool up);
uint32_t comms_sent_count();
uint32_t comms_failed_count();
bool comms_module_ok();
