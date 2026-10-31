#pragma once
#include <stddef.h>
#include <stdint.h>

bool kv_begin(const char *name_space);
bool kv_get_str(const char *key, char *out, size_t cap);
bool kv_put_str(const char *key, const char *value);
bool kv_get_u32(const char *key, uint32_t *out);
bool kv_put_u32(const char *key, uint32_t value);
bool kv_erase_all();
size_t kv_entry_count();
const char *kv_backend_name();
