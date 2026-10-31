#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cc_modem.h"
#include "cc_config.h"
#include "cc_log.h"

namespace {

enum RunResult { RUN_PENDING, RUN_OK, RUN_FAIL, RUN_TIMEOUT };

AtPort *port = 0;
ModemState state = MODEM_OFF;
uint32_t stateSince = 0;
ErrCode lastErr = ERR_NONE;
uint32_t failures = 0;
uint32_t backoffUntil = 0;
uint32_t curBackoffS = 0;

bool cmdActive = false;
uint32_t cmdStart = 0;
uint32_t cmdTimeout = 0;
char cmdExpect[24];
char lastLine[96];
bool expectSeen = false;

int attemptsInState = 0;
uint32_t nextRegPoll = 0;
uint32_t regStart = 0;
uint32_t lastRegCheck = 0;
bool registered = false;
int csqValue = -1;

int postStep = 0;
char postBuf[648];
size_t postLen = 0;
int postStatus = 0;
PostResult postResult = POST_NONE;
bool postDone = false;
uint32_t postStarted = 0;

void set_state(ModemState s, uint32_t now) {
  if (s != state) LOGD("MODEM", "%s -> %s", modem_state_name(state), modem_state_name(s));
  state = s;
  stateSince = now;
  cmdActive = false;
  attemptsInState = 0;
}

void fail(ErrCode code, uint32_t now, const char *why) {
  lastErr = code;
  failures++;
  uint32_t b = config().backoff_init_s;
  for (uint32_t i = 1; i < failures && b < config().backoff_max_s; ++i) b *= 2;
  if (b > config().backoff_max_s) b = config().backoff_max_s;
  curBackoffS = b;
  backoffUntil = now + b * 1000u;
  registered = false;
  LOGW("MODEM", code, "%s, power-cycle retry in %lus (failure %lu)", why, (unsigned long)b, (unsigned long)failures);
  set_state(MODEM_FAILED_BACKOFF, now);
}

void cmd_begin(uint32_t now, const char *cmd, const char *expect, uint32_t timeout) {
  snprintf(cmdExpect, sizeof(cmdExpect), "%s", expect);
  lastLine[0] = '\0';
  expectSeen = false;
  cmdStart = now;
  cmdTimeout = timeout;
  cmdActive = true;
  port->write_line(now, cmd);
}

RunResult cmd_poll(uint32_t now) {
  char line[96];
  while (port->read_line(now, line, sizeof(line))) {
    if (line[0] == '\0') continue;
    if (strstr(line, "ERROR") != 0) {
      snprintf(lastLine, sizeof(lastLine), "%s", line);
      cmdActive = false;
      return RUN_FAIL;
    }
    if (strstr(line, cmdExpect) != 0) {
      snprintf(lastLine, sizeof(lastLine), "%s", line);
      expectSeen = true;
      cmdActive = false;
      return RUN_OK;
    }
    snprintf(lastLine, sizeof(lastLine), "%s", line);
  }
  if ((now - cmdStart) >= cmdTimeout) {
    cmdActive = false;
    return RUN_TIMEOUT;
  }
  return RUN_PENDING;
}

bool parse_reg(const char *line) {
  const char *p = strstr(line, "+CEREG:");
  if (!p) return false;
  const char *c = strchr(p, ',');
  if (!c) return false;
  int stat = atoi(c + 1);
  return stat == 1 || stat == 5;
}

void post_finish(PostResult r, int status) {
  postResult = r;
  postStatus = status;
  postDone = true;
}

}

const char *modem_state_name(ModemState s) {
  switch (s) {
    case MODEM_OFF:            return "OFF";
    case MODEM_POWERING:       return "POWERING";
    case MODEM_AT_CHECK:       return "AT_CHECK";
    case MODEM_ECHO_OFF:       return "ECHO_OFF";
    case MODEM_SIM_CHECK:      return "SIM_CHECK";
    case MODEM_SIGNAL:         return "SIGNAL";
    case MODEM_REGISTERING:    return "REGISTERING";
    case MODEM_PDP_DEFINE:     return "PDP_DEFINE";
    case MODEM_PDP_ACTIVATE:   return "PDP_ACTIVATE";
    case MODEM_READY:          return "READY";
    case MODEM_POSTING:        return "POSTING";
    default:                   return "FAILED_BACKOFF";
  }
}

void modem_init(AtPort *p, uint32_t now) {
  port = p;
  failures = 0;
  lastErr = ERR_NONE;
  registered = false;
  csqValue = -1;
  postDone = false;
  postResult = POST_NONE;
  set_state(MODEM_OFF, now);
}

ModemState modem_state() { return state; }
bool modem_ready() { return state == MODEM_READY; }
bool modem_registered() { return registered; }
ErrCode modem_last_error() { return lastErr; }
uint32_t modem_failures() { return failures; }
uint32_t modem_backoff_s() { return curBackoffS; }

bool modem_csq(int *csq) {
  if (csqValue < 0 || csqValue > 31) return false;
  *csq = csqValue;
  return true;
}

void modem_update(uint32_t now) {
  if (!port) return;
  const Timing &t = timing();
  char cmd[160];
  RunResult r;

  switch (state) {
    case MODEM_OFF:
      port->power_on(now);
      set_state(MODEM_POWERING, now);
      break;

    case MODEM_POWERING:
      if ((now - stateSince) >= t.modem_boot_wait_ms) set_state(MODEM_AT_CHECK, now);
      break;

    case MODEM_AT_CHECK:
      if (!cmdActive) cmd_begin(now, "AT", "OK", t.modem_cmd_timeout_ms);
      r = cmd_poll(now);
      if (r == RUN_OK) set_state(MODEM_ECHO_OFF, now);
      else if (r == RUN_FAIL || r == RUN_TIMEOUT) {
        if (++attemptsInState >= 5) fail(ERR_MODEM_NO_RESPONSE, now, "no answer to AT");
      }
      break;

    case MODEM_ECHO_OFF:
      if (!cmdActive) cmd_begin(now, "ATE0", "OK", t.modem_cmd_timeout_ms);
      r = cmd_poll(now);
      if (r == RUN_OK) set_state(MODEM_SIM_CHECK, now);
      else if (r != RUN_PENDING) fail(ERR_MODEM_AT_ERROR, now, "ATE0 failed");
      break;

    case MODEM_SIM_CHECK:
      if (!cmdActive) cmd_begin(now, "AT+CPIN?", "+CPIN: READY", t.modem_cmd_timeout_ms);
      r = cmd_poll(now);
      if (r == RUN_OK) set_state(MODEM_SIGNAL, now);
      else if (r != RUN_PENDING) fail(ERR_MODEM_SIM_NOT_READY, now, "SIM not ready");
      break;

    case MODEM_SIGNAL:
      if (!cmdActive) cmd_begin(now, "AT+CSQ", "+CSQ:", t.modem_cmd_timeout_ms);
      r = cmd_poll(now);
      if (r == RUN_OK) {
        const char *p = strstr(lastLine, "+CSQ:");
        csqValue = p ? atoi(p + 5) : -1;
        if (csqValue == 99) csqValue = -1;
        regStart = now;
        nextRegPoll = now;
        set_state(MODEM_REGISTERING, now);
      } else if (r != RUN_PENDING) {
        fail(ERR_MODEM_AT_ERROR, now, "AT+CSQ failed");
      }
      break;

    case MODEM_REGISTERING:
      if (!cmdActive) {
        if (now < nextRegPoll) break;
        cmd_begin(now, "AT+CEREG?", "+CEREG:", t.modem_cmd_timeout_ms);
      }
      r = cmd_poll(now);
      if (r == RUN_OK) {
        if (parse_reg(lastLine)) {
          registered = true;
          set_state(MODEM_PDP_DEFINE, now);
        } else {
          nextRegPoll = now + 2000;
        }
      } else if (r != RUN_PENDING) {
        nextRegPoll = now + 2000;
      }
      if (state == MODEM_REGISTERING && (now - regStart) >= t.modem_register_timeout_ms) {
        fail(ERR_MODEM_NOT_REGISTERED, now, "network registration timeout");
      }
      break;

    case MODEM_PDP_DEFINE:
      if (!cmdActive) {
        snprintf(cmd, sizeof(cmd), "AT+CGDCONT=1,\"IP\",\"%s\"", config().apn);
        cmd_begin(now, cmd, "OK", t.modem_cmd_timeout_ms);
      }
      r = cmd_poll(now);
      if (r == RUN_OK) set_state(MODEM_PDP_ACTIVATE, now);
      else if (r != RUN_PENDING) fail(ERR_MODEM_DATA_SESSION, now, "PDP context define failed");
      break;

    case MODEM_PDP_ACTIVATE:
      if (!cmdActive) cmd_begin(now, "AT+CGACT=1,1", "OK", t.modem_register_timeout_ms);
      r = cmd_poll(now);
      if (r == RUN_OK) {
        if (failures > 0) LOGI("MODEM", "recovered after %lu failure(s)", (unsigned long)failures);
        failures = 0;
        lastErr = ERR_NONE;
        lastRegCheck = now;
        set_state(MODEM_READY, now);
      } else if (r != RUN_PENDING) {
        fail(ERR_MODEM_DATA_SESSION, now, "PDP context activate failed");
      }
      break;

    case MODEM_READY:
      if (!cmdActive) {
        if ((now - lastRegCheck) < t.modem_reg_check_ms) break;
        cmd_begin(now, "AT+CEREG?", "+CEREG:", t.modem_cmd_timeout_ms);
      }
      r = cmd_poll(now);
      if (r == RUN_OK) {
        lastRegCheck = now;
        if (!parse_reg(lastLine)) {
          registered = false;
          lastErr = ERR_MODEM_NOT_REGISTERED;
          LOGW("MODEM", ERR_MODEM_NOT_REGISTERED, "network registration lost");
          regStart = now;
          nextRegPoll = now + 2000;
          set_state(MODEM_REGISTERING, now);
        }
      } else if (r != RUN_PENDING) {
        lastRegCheck = now;
      }
      break;

    case MODEM_POSTING: {
      if (now - postStarted > t.modem_http_timeout_ms + 15000u) {
        post_finish(POST_NO_SERVER, 0);
        set_state(MODEM_READY, now);
        break;
      }
      if (!cmdActive) {
        switch (postStep) {
          case 0: cmd_begin(now, "AT+HTTPINIT", "OK", t.modem_cmd_timeout_ms); break;
          case 1:
            snprintf(cmd, sizeof(cmd), "AT+HTTPPARA=\"URL\",\"%s\"", config().server_url);
            cmd_begin(now, cmd, "OK", t.modem_cmd_timeout_ms);
            break;
          case 2: cmd_begin(now, "AT+HTTPPARA=\"CONTENT\",\"application/json\"", "OK", t.modem_cmd_timeout_ms); break;
          case 3:
            snprintf(cmd, sizeof(cmd), "AT+HTTPDATA=%u,5000", (unsigned)postLen);
            cmd_begin(now, cmd, "DOWNLOAD", t.modem_cmd_timeout_ms);
            break;
          case 4:
            cmd_begin(now, "", "OK", t.modem_cmd_timeout_ms);
            port->write_raw(now, postBuf, postLen);
            break;
          case 5: cmd_begin(now, "AT+HTTPACTION=1", "+HTTPACTION:", t.modem_http_timeout_ms); break;
          default: cmd_begin(now, "AT+HTTPTERM", "OK", t.modem_cmd_timeout_ms); break;
        }
      }
      r = cmd_poll(now);
      if (r == RUN_PENDING) break;
      if (postStep == 6) {
        set_state(MODEM_READY, now);
        break;
      }
      if (r != RUN_OK) {
        if (postStep == 5 && r == RUN_TIMEOUT) post_finish(POST_NO_SERVER, 0);
        else post_finish(POST_NO_SERVER, 0);
        postStep = 6;
        break;
      }
      if (postStep == 5) {
        const char *p = strstr(lastLine, "+HTTPACTION:");
        int method = 0, code = 0;
        if (p && sscanf(p + 12, " %d,%d", &method, &code) == 2) {
          if (code >= 200 && code < 300) post_finish(POST_ACKED, code);
          else post_finish(POST_HTTP_ERROR, code);
        } else {
          post_finish(POST_NO_SERVER, 0);
        }
      }
      postStep++;
      break;
    }

    case MODEM_FAILED_BACKOFF:
      if (now >= backoffUntil) set_state(MODEM_OFF, now);
      break;
  }
}

bool modem_post_begin(uint32_t now, const char *payload, size_t len) {
  if (state != MODEM_READY || len >= sizeof(postBuf)) return false;
  memcpy(postBuf, payload, len);
  postBuf[len] = '\0';
  postLen = len;
  postStep = 0;
  postDone = false;
  postResult = POST_PENDING;
  postStatus = 0;
  postStarted = now;
  set_state(MODEM_POSTING, now);
  return true;
}

PostResult modem_post_poll(int *http_status) {
  if (http_status) *http_status = postStatus;
  if (!postDone) return state == MODEM_POSTING ? POST_PENDING : POST_NO_NETWORK;
  if (state == MODEM_POSTING) return POST_PENDING;
  PostResult r = postResult;
  postDone = false;
  postResult = POST_NONE;
  return r;
}

void modem_post_abort() {
  if (state == MODEM_POSTING) {
    postDone = false;
    postResult = POST_NONE;
    set_state(MODEM_READY, stateSince);
  }
}
