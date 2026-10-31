#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mock_at_port.h"

MockAtPort::MockAtPort()
    : responds(true), sim_ready(true), registered(true), data_ok(true), server(SERVER_OK), powered_ms(0),
      power_on_count(0), received_count(0), commands_seen(0), count_(0), booted_(false) {
  memset(received_ids, 0, sizeof(received_ids));
  memset(received_payload, 0, sizeof(received_payload));
  memset(received_status, 0, sizeof(received_status));
  last_cmd[0] = '\0';
}

void MockAtPort::push(uint32_t readyMs, const char *text) {
  if (count_ >= 24) return;
  q_[count_].readyMs = readyMs;
  snprintf(q_[count_].text, sizeof(q_[count_].text), "%s", text);
  count_++;
}

void MockAtPort::power_on(uint32_t now) {
  power_on_count++;
  powered_ms = now;
  booted_ = true;
  count_ = 0;
}

void MockAtPort::write_line(uint32_t now, const char *cmd) {
  commands_seen++;
  snprintf(last_cmd, sizeof(last_cmd), "%s", cmd);
  if (!responds || !booted_ || cmd[0] == '\0') return;
  const uint32_t d = now + 20;
  if (strcmp(cmd, "AT") == 0 || strcmp(cmd, "ATE0") == 0) {
    push(d, "OK");
  } else if (strncmp(cmd, "AT+CPIN?", 8) == 0) {
    if (sim_ready) { push(d, "+CPIN: READY"); push(d, "OK"); }
    else push(d, "+CME ERROR: SIM not inserted");
  } else if (strncmp(cmd, "AT+CSQ", 6) == 0) {
    push(d, "+CSQ: 18,99");
    push(d, "OK");
  } else if (strncmp(cmd, "AT+CEREG?", 9) == 0) {
    push(d, registered ? "+CEREG: 0,1" : "+CEREG: 0,2");
    push(d, "OK");
  } else if (strncmp(cmd, "AT+CGDCONT", 10) == 0) {
    push(d, "OK");
  } else if (strncmp(cmd, "AT+CGACT", 8) == 0) {
    push(d, data_ok ? "OK" : "ERROR");
  } else if (strncmp(cmd, "AT+HTTPINIT", 11) == 0 || strncmp(cmd, "AT+HTTPPARA", 11) == 0 ||
             strncmp(cmd, "AT+HTTPTERM", 11) == 0) {
    push(d, "OK");
  } else if (strncmp(cmd, "AT+HTTPDATA", 11) == 0) {
    push(d, "DOWNLOAD");
  } else if (strncmp(cmd, "AT+HTTPACTION", 13) == 0) {
    push(d, "OK");
    char line[64];
    if (server == SERVER_OK) {
      snprintf(line, sizeof(line), "+HTTPACTION: 1,200,16");
      push(now + 300, line);
    } else if (server == SERVER_ERROR_503) {
      snprintf(line, sizeof(line), "+HTTPACTION: 1,503,0");
      push(now + 300, line);
    } else if (server == SERVER_REJECT_400) {
      snprintf(line, sizeof(line), "+HTTPACTION: 1,400,0");
      push(now + 300, line);
    }
  }
}

void MockAtPort::write_raw(uint32_t now, const char *data, size_t len) {
  if (!responds || !booted_) return;
  push(now + 20, "OK");
  if (server == SERVER_DOWN) return;
  if (received_count < 48) {
    uint32_t i = received_count++;
    size_t n = len < sizeof(received_payload[i]) - 1 ? len : sizeof(received_payload[i]) - 1;
    memcpy(received_payload[i], data, n);
    received_payload[i][n] = '\0';
    received_status[i] = server == SERVER_OK ? 200 : (server == SERVER_ERROR_503 ? 503 : 400);
    const char *key = "\"event_id\":\"";
    const char *id = strstr(received_payload[i], key);
    if (id) {
      id += strlen(key);
      const char *e = strchr(id, '"');
      size_t m = e ? (size_t)(e - id) : 0;
      if (m >= sizeof(received_ids[i])) m = sizeof(received_ids[i]) - 1;
      memcpy(received_ids[i], id, m);
      received_ids[i][m] = '\0';
    } else {
      const char *sq = strstr(received_payload[i], "\"sequence_number\":");
      snprintf(received_ids[i], sizeof(received_ids[i]), "seq:%d", sq ? atoi(sq + 18) : -1);
    }
  }
}

bool MockAtPort::read_line(uint32_t now, char *out, size_t cap) {
  for (int i = 0; i < count_; ++i) {
    if (q_[i].readyMs <= now) {
      snprintf(out, cap, "%s", q_[i].text);
      for (int j = i; j + 1 < count_; ++j) q_[j] = q_[j + 1];
      count_--;
      return true;
    }
  }
  return false;
}
