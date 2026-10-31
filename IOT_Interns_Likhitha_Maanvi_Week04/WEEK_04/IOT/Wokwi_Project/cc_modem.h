#pragma once
#include <stddef.h>
#include <stdint.h>
#include "cc_errors.h"

class AtPort {
 public:
  virtual ~AtPort() {}
  virtual void power_on(uint32_t now_ms) = 0;
  virtual void write_line(uint32_t now_ms, const char *cmd) = 0;
  virtual void write_raw(uint32_t now_ms, const char *data, size_t len) = 0;
  virtual bool read_line(uint32_t now_ms, char *out, size_t cap) = 0;
};

enum ModemState : uint8_t {
  MODEM_OFF = 0,
  MODEM_POWERING,
  MODEM_AT_CHECK,
  MODEM_ECHO_OFF,
  MODEM_SIM_CHECK,
  MODEM_SIGNAL,
  MODEM_REGISTERING,
  MODEM_PDP_DEFINE,
  MODEM_PDP_ACTIVATE,
  MODEM_READY,
  MODEM_POSTING,
  MODEM_FAILED_BACKOFF
};

enum PostResult : uint8_t {
  POST_NONE = 0,
  POST_PENDING,
  POST_ACKED,
  POST_HTTP_ERROR,
  POST_NO_SERVER,
  POST_NO_NETWORK
};

void modem_init(AtPort *port, uint32_t now_ms);
void modem_update(uint32_t now_ms);
ModemState modem_state();
const char *modem_state_name(ModemState s);
bool modem_ready();
bool modem_post_begin(uint32_t now_ms, const char *payload, size_t len);
PostResult modem_post_poll(int *http_status);
void modem_post_abort();
bool modem_csq(int *csq);
bool modem_registered();
ErrCode modem_last_error();
uint32_t modem_failures();
uint32_t modem_backoff_s();
