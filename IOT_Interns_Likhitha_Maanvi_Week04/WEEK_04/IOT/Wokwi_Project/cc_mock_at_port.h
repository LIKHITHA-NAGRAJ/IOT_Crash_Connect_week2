#pragma once
#include "cc_modem.h"

enum ServerMode : uint8_t {
  SERVER_OK = 0,
  SERVER_DOWN,
  SERVER_ERROR_503,
  SERVER_REJECT_400
};

struct MockLine {
  uint32_t readyMs;
  char text[96];
};

class MockAtPort : public AtPort {
 public:
  MockAtPort();
  void power_on(uint32_t now_ms) override;
  void write_line(uint32_t now_ms, const char *cmd) override;
  void write_raw(uint32_t now_ms, const char *data, size_t len) override;
  bool read_line(uint32_t now_ms, char *out, size_t cap) override;

  bool responds;
  bool sim_ready;
  bool registered;
  bool data_ok;
  ServerMode server;
  uint32_t powered_ms;
  uint32_t power_on_count;
  uint32_t received_count;
  char received_ids[48][64];
  char received_payload[48][640];
  int received_status[48];
  uint32_t commands_seen;
  char last_cmd[160];

 private:
  void push(uint32_t readyMs, const char *text);
  MockLine q_[24];
  int count_;
  bool booted_;
};
