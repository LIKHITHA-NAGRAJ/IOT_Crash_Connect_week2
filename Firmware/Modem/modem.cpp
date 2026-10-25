#include "modem.h"
#include "config.h"
#include <Arduino.h>
#include <stdio.h>
#include <string.h>

namespace Modem {

  // The real transport (UART to the A7670C) does not exist yet.
  static const bool REAL_TRANSPORT_AVAILABLE = false;

  static State        state = NOT_STARTED;
  static MockScenario scenario = MOCK_ALL_OK;
  static const char*  apnName = nullptr;
  static Status       st;
  static uint32_t     enteredMs = 0;
  static uint32_t     lastAttemptMs = 0;
  static int          regPolls = 0;

  // ------------------------------------------------------------------ mock
  // Pretends to be the modem. Returns false = "no response" (a timeout).
  static bool mockExchange(const char* cmd, char* resp, size_t n) {
    if (scenario == MOCK_NO_RESPONSE) return false;
    const char* r = "ERROR";
    if (strcmp(cmd, "AT") == 0) {
      r = "OK";
    } else if (strcmp(cmd, "AT+CPIN?") == 0) {
      r = (scenario == MOCK_NO_SIM) ? "+CPIN: NOT INSERTED" : "+CPIN: READY";
    } else if (strcmp(cmd, "AT+CEREG?") == 0) {
      regPolls++;
      if (scenario == MOCK_REG_DENIED)      r = "+CEREG: 0,3";
      else if (scenario == MOCK_REG_NEVER)  r = "+CEREG: 0,2";
      else                                  r = (regPolls >= 3) ? "+CEREG: 0,1" : "+CEREG: 0,2";
    } else if (strcmp(cmd, "AT+CSQ") == 0) {
      r = (scenario == MOCK_WEAK_SIGNAL) ? "+CSQ: 3,99" : "+CSQ: 18,99";
    } else if (strncmp(cmd, "AT+CGDCONT=", 11) == 0) {
      r = "OK";
    } else if (strcmp(cmd, "AT+NETOPEN") == 0) {
      r = (scenario == MOCK_DATA_FAIL) ? "+NETOPEN: 1" : "+NETOPEN: 0";
    }
    strncpy(resp, r, n - 1);
    resp[n - 1] = '\0';
    return true;
  }

  // One command/response exchange. Only the mock exists for now.
  static bool exchange(const char* cmd, char* resp, size_t n) {
    Serial.print("[MODEM][MOCK] > "); Serial.println(cmd);
    bool got = REAL_TRANSPORT_AVAILABLE ? false : mockExchange(cmd, resp, n);
    if (got) { Serial.print("[MODEM][MOCK] < "); Serial.println(resp); }
    else     { Serial.println("[MODEM][MOCK] < (no response: timeout)"); resp[0] = '\0'; }
    return got;
  }

  // ------------------------------------------------------------- helpers
  static bool elapsed(uint32_t since, uint32_t ms) { return millis() - since >= ms; }

  static void enter(State s) {
    Serial.print("[MODEM][MOCK] state: "); Serial.print(stateName(state));
    Serial.print(" -> "); Serial.println(stateName(s));
    state = s;
    st.state = s;
    enteredMs = millis();
    if (s < FAILED_NO_RESPONSE) st.attempts = 0;   // keep the attempt count visible in failure states
  }

  // True when a new attempt should be made now (first one at once, then every retry delay).
  static bool due() {
    return st.attempts == 0 || elapsed(lastAttemptMs, CONFIG.comms.retryDelayMs);
  }
  static void markAttempt() { lastAttemptMs = millis(); st.attempts++; }
  static bool outOfRetries() { return st.attempts >= CONFIG.comms.maxRetries; }

  // -------------------------------------------------------------- public
  void begin(const char* apn, MockScenario sc) {
    apnName = apn;
    scenario = sc;
    regPolls = 0;
    memset(&st, 0, sizeof st);
    st.rssi = -1;
    st.simulated = true;
    state = NOT_STARTED;
    st.state = NOT_STARTED;
    Serial.println("[MODEM] MOCK modem initialised. Responses are SIMULATED, not from a real A7670C.");
  }

  void powerOn() {
    if (state == NOT_STARTED) enter(POWER_UP);     // a real build would drive PWRKEY here (pin PENDING)
  }

  void step() {
    char resp[48];
    switch (state) {
      case POWER_UP:
        if (elapsed(enteredMs, CONFIG.modem.powerUpMs)) enter(AT_CHECK);
        break;

      case AT_CHECK:
        if (!due()) break;
        markAttempt();
        if (exchange("AT", resp, sizeof resp) && strcmp(resp, "OK") == 0) enter(SIM_CHECK);
        else if (outOfRetries()) enter(FAILED_NO_RESPONSE);
        break;

      case SIM_CHECK:
        if (!due()) break;
        markAttempt();
        if (exchange("AT+CPIN?", resp, sizeof resp) && strcmp(resp, "+CPIN: READY") == 0) {
          st.simReady = true;
          enter(REGISTERING);
        } else if (outOfRetries()) {
          enter(FAILED_NO_SIM);
        }
        break;

      case REGISTERING:
        if (elapsed(enteredMs, CONFIG.modem.registerTimeoutMs)) { enter(FAILED_REG_TIMEOUT); break; }
        if (!due()) break;
        markAttempt();
        if (exchange("AT+CEREG?", resp, sizeof resp)) {
          int stat = -1;
          if (sscanf(resp, "+CEREG: %*d,%d", &stat) == 1) {
            if (stat == 1 || stat == 5) { st.registered = true; enter(SIGNAL_CHECK); }   // home or roaming
            else if (stat == 3) enter(FAILED_REG_DENIED);
            // other values (0, 2, 4): keep polling until the timeout
          }
        }
        break;

      case SIGNAL_CHECK: {
        markAttempt();
        int rssi = -1, ber = -1;
        if (exchange("AT+CSQ", resp, sizeof resp) && sscanf(resp, "+CSQ: %d,%d", &rssi, &ber) == 2) {
          st.rssi = rssi;
          // POLICY PENDING CONFIRMATION: a weak signal is only flagged here, it does not stop bring-up.
          st.signalWeak = (rssi == 99 || rssi < CONFIG.modem.minRssi);
        }
        enter(APN_CONFIG);
        break;
      }

      case APN_CONFIG: {
        if (apnName == nullptr || strcmp(apnName, "PENDING") == 0 || apnName[0] == '\0') {
          Serial.println("[MODEM] APN is still PENDING CONFIRMATION: cannot configure the data context");
          enter(FAILED_APN_PENDING);
          break;
        }
        if (!due()) break;
        markAttempt();
        char cmd[80];
        snprintf(cmd, sizeof cmd, "AT+CGDCONT=1,\"IP\",\"%s\"", apnName);
        if (exchange(cmd, resp, sizeof resp) && strcmp(resp, "OK") == 0) enter(DATA_SESSION);
        else if (outOfRetries()) enter(FAILED_DATA_SESSION);
        break;
      }

      case DATA_SESSION:
        if (!due()) break;
        markAttempt();
        // "AT+NETOPEN" stands in for starting the modem's TCP/IP service. The real command and
        // the transport (TCP/MQTT versus PPP) are PENDING Technical Lead confirmation.
        if (exchange("AT+NETOPEN", resp, sizeof resp) && strcmp(resp, "+NETOPEN: 0") == 0) {
          st.dataSession = true;
          enter(READY_SIMULATED);
          Serial.println("[MODEM] READY_SIMULATED: mock only. NOT a real registration or data session.");
        } else if (outOfRetries()) {
          enter(FAILED_DATA_SESSION);
        }
        break;

      default:
        break;   // NOT_STARTED and terminal states do nothing
    }
  }

  Status getStatus() { return st; }

  bool isFailed() { return state >= FAILED_NO_RESPONSE; }
  bool isTerminal() { return state == READY_SIMULATED || isFailed(); }

  const char* stateName(State s) {
    switch (s) {
      case NOT_STARTED: return "NOT_STARTED";
      case POWER_UP: return "POWER_UP";
      case AT_CHECK: return "AT_CHECK";
      case SIM_CHECK: return "SIM_CHECK";
      case REGISTERING: return "REGISTERING";
      case SIGNAL_CHECK: return "SIGNAL_CHECK";
      case APN_CONFIG: return "APN_CONFIG";
      case DATA_SESSION: return "DATA_SESSION";
      case READY_SIMULATED: return "READY_SIMULATED";
      case FAILED_NO_RESPONSE: return "FAILED_NO_RESPONSE";
      case FAILED_NO_SIM: return "FAILED_NO_SIM";
      case FAILED_REG_DENIED: return "FAILED_REG_DENIED";
      case FAILED_REG_TIMEOUT: return "FAILED_REG_TIMEOUT";
      case FAILED_APN_PENDING: return "FAILED_APN_PENDING";
      case FAILED_DATA_SESSION: return "FAILED_DATA_SESSION";
    }
    return "UNKNOWN";
  }
}
