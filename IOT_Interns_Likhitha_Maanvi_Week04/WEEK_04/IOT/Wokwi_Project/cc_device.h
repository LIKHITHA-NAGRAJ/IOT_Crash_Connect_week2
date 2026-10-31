#pragma once
#include <stdint.h>
#include "cc_types.h"

uint32_t epoch_from_ymdhms(int y, int mo, int d, int h, int mi, int s);
void clock_reset();
void clock_set(uint32_t epoch, TimeSource src, uint32_t now_ms);
bool clock_now(uint32_t now_ms, uint32_t *epoch, TimeSource *src);

const char *reset_reason_name();
void reset_reason_sim_set(const char *name);

bool wd_init(uint32_t timeout_s, uint32_t now_ms);
void wd_feed(uint32_t now_ms);
bool wd_sim_expired(uint32_t now_ms);
