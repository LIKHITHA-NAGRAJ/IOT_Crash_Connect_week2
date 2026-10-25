#pragma once
// config.h - all tunable settings live here, NOT in the logic.
// Values are PLACEHOLDERS unless stated. "PENDING CONFIRMATION" = needs team confirmation.
#include <stdint.h>

struct IntervalConfig {
  uint32_t gnssReadMs;     // how often simulated GNSS data is processed
  uint32_t imuReadMs;      // how often the IMU is read
  uint32_t telemetryMs;    // how often telemetry is sent
  uint32_t statusMs;       // how often device status is printed
};

struct CommsConfig {
  const char* apn;         // PENDING CONFIRMATION
  const char* serverHost;  // PENDING CONFIRMATION
  uint16_t    serverPort;  // PENDING CONFIRMATION
  uint8_t     maxRetries;
  uint32_t    retryDelayMs;
};

struct EventConfig {
  float crashThresholdG;   // PLACEHOLDER, PENDING CONFIRMATION
};

struct TimeoutConfig {
  uint32_t gpsAcquireMs;
  uint32_t networkConnectMs;
  uint32_t gnssStaleMs;
  uint32_t eventHoldMs;
};

struct PinConfig {         // -1 = PENDING CONFIRMATION (pins not decided)
  int gnssRx, gnssTx;
  int imuSda, imuScl;
  int modemRx, modemTx;
};

struct GnssConfig {
  uint8_t minSatellites;    // PLACEHOLDER. PENDING CONFIRMATION: minimum satellites for a usable fix
  float   maxHdop;          // PLACEHOLDER. PENDING CONFIRMATION: maximum HDOP for a usable fix
};

struct ModemConfig {
  uint32_t powerUpMs;        // PLACEHOLDER: wait after power-up before the first AT command
  uint32_t registerTimeoutMs;// PLACEHOLDER: give up waiting for network registration
  int      minRssi;          // PLACEHOLDER: CSQ value below this is treated as weak signal
};

// Staged accident-detection parameters.
// ALL VALUES ARE PLACEHOLDERS chosen to exercise the software with SIMULATED data.
// They are NOT validated accident-detection settings. Real values need logged vehicle data.
struct AccidentConfig {
  // Stage 1: candidate trigger
  float    impactThresholdG;     // magnitude (g, includes gravity) that starts an impact
  uint32_t impactMinMs;          // magnitude must stay above the threshold at least this long
  bool     jerkEnabled;          // also allow a trigger on a very fast magnitude change
  float    jerkThresholdGps;     // g per second
  // Stage 2: orientation change
  uint32_t gravityTauMs;         // time constant of the slow gravity-vector estimate
  float    quietBandG;           // gravity estimate only updates when |magnitude - 1 g| is below this
  float    gyroSpikeDps;         // peak angular rate after impact that counts as a spike
  float    orientationChangeDeg; // angle between gravity before and after that counts as a change
  uint32_t orientationSampleMs;  // length of the final window used to measure gravity after impact
  bool     requireOrientation;   // true = orientation change or gyro spike is required to confirm
  // Stage 3: stillness and GPS speed drop
  uint32_t settleMs;             // ignore the first part after impact (ringing, tumbling)
  float    stillBandG;           // |magnitude - 1 g| at or below this counts as quiet
  float    stillGyroDps;         // angular rate at or below this counts as quiet
  uint32_t stillnessMinMs;       // quiet must last at least this long before the window ends
  float    minPreSpeedKmh;       // vehicle must have been at least this fast (when speed is known)
  float    speedDropDeltaKmh;    // required speed drop
  float    postSpeedMaxKmh;      // speed after the event must be at or below this
  // Stage 4: confirmation window
  uint32_t confirmWindowMs;      // time from candidate to final decision
  uint32_t cooldownMs;           // ignore new candidates for this long after a decision
};

// Manual SOS button (long-press). PLACEHOLDERS: the button, its pin and its circuit are not confirmed.
struct SosConfig {
  uint32_t debounceMs;           // raw level must be stable this long
  uint32_t longPressMs;          // hold time that triggers an SOS (example 2 to 3 seconds)
  uint32_t cooldownMs;           // ignore new presses for this long after a trigger
};

// Event creation and transmission. PLACEHOLDERS, PENDING CONFIRMATION with Full Stack and the Technical Lead.
struct EventsConfig {
  const char* schemaVersion;     // "1.0" in the sample event model; added fields may need a new version
  bool        isoTimestamp;      // false = integer UTC epoch seconds (Technical Lead), true = ISO 8601 text (sample)
  uint32_t    maxLastKnownAgeS;  // a last-known location older than this is reported as unavailable
  uint32_t    retryBaseMs;       // first retry delay
  uint32_t    retryMaxMs;        // retry delay cap (exponential backoff)
};

struct DeviceConfig {
  const char*    deviceId;
  bool           simulateHardware;   // true = recorded/simulated inputs, no hardware
  IntervalConfig intervals;
  CommsConfig    comms;
  EventConfig    event;
  TimeoutConfig  timeouts;
  PinConfig      pins;
  GnssConfig     gnss;
  ModemConfig    modem;
  AccidentConfig accident;
  SosConfig      sos;
  EventsConfig   events;
};

extern const DeviceConfig CONFIG;