#pragma once
#include <stdint.h>

enum ErrSeverity : uint8_t {
  SEV_INFO = 0,
  SEV_WARNING,
  SEV_ERROR,
  SEV_CRITICAL
};

#define CC_ERROR_TABLE(X) \
  X(ERR_NONE,                 0,   SEV_INFO,     "No error") \
  X(ERR_CONFIG_INVALID,     100, SEV_CRITICAL, "Stored configuration is unusable") \
  X(ERR_CONFIG_DEFAULTED,   101, SEV_WARNING,  "A configuration value was missing or out of range, default used") \
  X(ERR_STORAGE_FAILED,     102, SEV_ERROR,    "Non-volatile storage read or write failed") \
  X(ERR_GNSS_NO_DATA,       200, SEV_WARNING,  "No valid NMEA data received from the GNSS receiver") \
  X(ERR_GNSS_NO_FIX,        201, SEV_WARNING,  "GNSS receiver reports no position fix") \
  X(ERR_GNSS_STALE,         202, SEV_WARNING,  "GNSS data stopped arriving") \
  X(ERR_GNSS_CHECKSUM,      203, SEV_WARNING,  "NMEA sentence rejected, checksum mismatch") \
  X(ERR_IMU_UNAVAILABLE,    300, SEV_ERROR,    "IMU is not available") \
  X(ERR_IMU_SAMPLE_INVALID, 301, SEV_WARNING,  "IMU sample invalid and ignored") \
  X(ERR_MODEM_NO_RESPONSE,  400, SEV_ERROR,    "Modem does not answer AT commands") \
  X(ERR_MODEM_SIM_NOT_READY,401, SEV_ERROR,    "SIM card not ready") \
  X(ERR_MODEM_NOT_REGISTERED,402,SEV_WARNING,  "Modem not registered on the network") \
  X(ERR_MODEM_DATA_SESSION, 403, SEV_ERROR,    "Data session could not be set up") \
  X(ERR_MODEM_AT_ERROR,     404, SEV_ERROR,    "Modem returned ERROR or an unexpected reply") \
  X(ERR_TX_NO_NETWORK,      500, SEV_WARNING,  "Transmission waiting, no network") \
  X(ERR_TX_NO_SERVER,       501, SEV_WARNING,  "Server unreachable or no HTTP answer, will retry") \
  X(ERR_TX_REJECTED,        502, SEV_ERROR,    "Server rejected the payload, record dropped") \
  X(ERR_TX_ACK_TIMEOUT,     503, SEV_WARNING,  "No acknowledgement within the timeout, will retry") \
  X(ERR_QUEUE_DROP_NEWEST,  600, SEV_WARNING,  "Queue full, newest record dropped") \
  X(ERR_QUEUE_DROP_OLDEST,  601, SEV_WARNING,  "Queue full, oldest record dropped") \
  X(ERR_PAYLOAD_TOO_LARGE,  602, SEV_ERROR,    "Payload does not fit its buffer, not sent") \
  X(ERR_SELF_TEST_FAILED,   700, SEV_CRITICAL, "Start-up self-test failed") \
  X(ERR_WATCHDOG_RESET,     701, SEV_CRITICAL, "Device restarted by the watchdog") \
  X(ERR_TIME_UNAVAILABLE,   800, SEV_WARNING,  "No trusted UTC time available")

enum ErrCode : uint16_t {
#define CC_ERR_ENUM(name, num, sev, text) name = num,
  CC_ERROR_TABLE(CC_ERR_ENUM)
#undef CC_ERR_ENUM
};

const char *err_name(ErrCode c);
const char *err_text(ErrCode c);
ErrSeverity err_severity(ErrCode c);
const char *err_severity_name(ErrSeverity s);
int err_table_size();
bool err_table_at(int index, ErrCode *code, const char **name, ErrSeverity *sev, const char **text);
