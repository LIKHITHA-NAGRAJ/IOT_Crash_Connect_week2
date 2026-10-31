#include <Arduino.h>
#include <stdio.h>
#include <string.h>
#include "kv_store.h"

#if defined(ARDUINO_ARCH_ESP32)

#include <Preferences.h>

namespace {
Preferences prefs;
bool opened = false;
}

bool kv_begin(const char *name_space) {
  if (opened) return true;
  opened = prefs.begin(name_space, false);
  return opened;
}

bool kv_get_str(const char *key, char *out, size_t cap) {
  if (!opened || !prefs.isKey(key)) return false;
  size_t n = prefs.getString(key, out, cap);
  return n > 0;
}

bool kv_put_str(const char *key, const char *value) {
  return opened && prefs.putString(key, value) > 0;
}

bool kv_get_u32(const char *key, uint32_t *out) {
  if (!opened || !prefs.isKey(key)) return false;
  *out = prefs.getUInt(key, 0);
  return true;
}

bool kv_put_u32(const char *key, uint32_t value) {
  return opened && prefs.putUInt(key, value) > 0;
}

bool kv_erase_all() {
  return opened && prefs.clear();
}

size_t kv_entry_count() {
  return 0;
}

const char *kv_backend_name() {
  return "ESP32 NVS (Preferences)";
}

#else

namespace {

const size_t kMaxEntries = 40;

struct Entry {
  bool used;
  bool is_str;
  char key[24];
  char str[64];
  uint32_t num;
};

Entry table[kMaxEntries];
bool opened = false;

Entry *find(const char *key) {
  for (size_t i = 0; i < kMaxEntries; ++i) {
    if (table[i].used && strcmp(table[i].key, key) == 0) return &table[i];
  }
  return 0;
}

Entry *alloc(const char *key) {
  Entry *e = find(key);
  if (e) return e;
  for (size_t i = 0; i < kMaxEntries; ++i) {
    if (!table[i].used) {
      memset(&table[i], 0, sizeof(Entry));
      table[i].used = true;
      snprintf(table[i].key, sizeof(table[i].key), "%s", key);
      return &table[i];
    }
  }
  return 0;
}

}

bool kv_begin(const char *) {
  opened = true;
  return true;
}

bool kv_get_str(const char *key, char *out, size_t cap) {
  Entry *e = opened ? find(key) : 0;
  if (!e || !e->is_str || cap == 0) return false;
  snprintf(out, cap, "%s", e->str);
  return true;
}

bool kv_put_str(const char *key, const char *value) {
  Entry *e = opened ? alloc(key) : 0;
  if (!e) return false;
  e->is_str = true;
  snprintf(e->str, sizeof(e->str), "%s", value);
  return true;
}

bool kv_get_u32(const char *key, uint32_t *out) {
  Entry *e = opened ? find(key) : 0;
  if (!e || e->is_str) return false;
  *out = e->num;
  return true;
}

bool kv_put_u32(const char *key, uint32_t value) {
  Entry *e = opened ? alloc(key) : 0;
  if (!e) return false;
  e->is_str = false;
  e->num = value;
  return true;
}

bool kv_erase_all() {
  memset(table, 0, sizeof(table));
  return true;
}

size_t kv_entry_count() {
  size_t n = 0;
  for (size_t i = 0; i < kMaxEntries; ++i) {
    if (table[i].used) n++;
  }
  return n;
}

const char *kv_backend_name() {
  return "HOST SIMULATION (in-memory key-value table, stands in for ESP32 NVS)";
}

#endif
