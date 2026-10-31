#include <Arduino.h>
#include "sim_scenario.h"
#include "../config/config.h"
#include "../gnss/gnss.h"
#include "../imu/imu.h"
#include "../comms/comms.h"
#include "../sos/sos.h"
#include "../fault/fault.h"

namespace {

enum SimAction {
  SIM_GPS_SIGNAL,
  SIM_NETWORK,
  SIM_IMU_CASE,
  SIM_SOS,
  SIM_CRITICAL_FAULT,
  SIM_CORRUPT_SENTENCE,
  SIM_END
};

struct SimStep {
  uint32_t at_ms;
  SimAction action;
  int value;
  const char *note;
};

const SimStep kSteps[] = {
  {0,     SIM_GPS_SIGNAL,       0,                  "no GPS signal yet, receiver searching"},
  {0,     SIM_NETWORK,          0,                  "network down"},
  {0,     SIM_IMU_CASE,         IMU_CASE_STATIONARY, "IMU stationary"},
  {4000,  SIM_CORRUPT_SENTENCE, 0,                  "corrupted NMEA sentence arrives"},
  {5000,  SIM_GPS_SIGNAL,       1,                  "GPS signal available"},
  {9000,  SIM_NETWORK,          1,                  "network available"},
  {16000, SIM_IMU_CASE,         IMU_CASE_TILT,      "IMU tilt/rotation (below impact threshold)"},
  {19000, SIM_IMU_CASE,         IMU_CASE_MOVEMENT,  "IMU movement (below impact threshold)"},
  {22000, SIM_IMU_CASE,         IMU_CASE_IMPACT,    "IMU impact test values (above placeholder threshold)"},
  {24000, SIM_IMU_CASE,         IMU_CASE_STATIONARY, "IMU stationary again"},
  {34000, SIM_GPS_SIGNAL,       0,                  "GPS signal lost"},
  {40000, SIM_GPS_SIGNAL,       1,                  "GPS signal back"},
  {46000, SIM_NETWORK,          0,                  "network lost"},
  {53000, SIM_NETWORK,          1,                  "network back"},
  {56000, SIM_SOS,              1,                  "SOS button pressed"},
  {58000, SIM_SOS,              0,                  "SOS button released"},
  {66000, SIM_CRITICAL_FAULT,   1,                  "critical fault injected"},
  {70000, SIM_CRITICAL_FAULT,   0,                  "critical fault cleared"},
  {84000, SIM_END,              0,                  "scenario end"}
};

const int kStepCount = sizeof(kSteps) / sizeof(kSteps[0]);

const char *kGgaValidA   = "$GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47\r\n";
const char *kRmcValidA   = "$GPRMC,123519,A,4807.038,N,01131.000,E,022.4,084.4,230394,003.1,W*6A\r\n";
const char *kGgaValidB   = "$GPGGA,123520,4807.045,N,01131.012,E,1,08,0.9,545.6,M,46.9,M,,*46\r\n";
const char *kRmcValidB   = "$GPRMC,123520,A,4807.045,N,01131.012,E,021.9,084.9,230394,003.1,W*6A\r\n";
const char *kGgaNoFix    = "$GPGGA,123521,,,,,0,00,99.99,,,,,,*4E\r\n";
const char *kRmcNoFix    = "$GPRMC,123521,V,,,,,,,230394,,*38\r\n";
const char *kGgaCorrupt  = "$GPGGA,123522,4807.050,N,01131.020,E,1,08,0.9,545.7,M,46.9,M,,*00\r\n";

int nextStep = 0;
uint32_t startMs = 0;
uint32_t lastFeedMs = 0;
uint32_t feedCount = 0;
bool gpsSignal = false;
bool done = false;

void feed_gps_records() {
  if (gpsSignal) {
    bool useA = (feedCount % 2) == 0;
    gnss_feed_nmea(useA ? kGgaValidA : kGgaValidB);
    gnss_feed_nmea(useA ? kRmcValidA : kRmcValidB);
  } else {
    gnss_feed_nmea(kGgaNoFix);
    gnss_feed_nmea(kRmcNoFix);
  }
  feedCount++;
}

void apply(const SimStep &s, uint32_t now_ms) {
  Serial.printf("[SIM][TEST INPUT] t=%.1fs %s\n", (now_ms - startMs) / 1000.0, s.note);
  switch (s.action) {
    case SIM_GPS_SIGNAL:       gpsSignal = (s.value != 0); break;
    case SIM_NETWORK:          comms_sim_set_network(s.value != 0); break;
    case SIM_IMU_CASE:         imu_sim_set_case((ImuCase)s.value); break;
    case SIM_SOS:              sos_sim_set(s.value != 0); break;
    case SIM_CRITICAL_FAULT:
      if (s.value) fault_set(FAULT_SIM_CRITICAL);
      else fault_clear(FAULT_SIM_CRITICAL);
      break;
    case SIM_CORRUPT_SENTENCE: gnss_feed_nmea(kGgaCorrupt); break;
    case SIM_END:              done = true; break;
  }
}

}

void sim_init(uint32_t now_ms) {
  nextStep = 0;
  startMs = now_ms;
  lastFeedMs = now_ms;
  feedCount = 0;
  gpsSignal = false;
  done = false;
  Serial.println("[SIM] SIMULATED / TEST INPUTS ENABLED - not real GPS, IMU, SOS or network data");
}

void sim_update(uint32_t now_ms) {
  uint32_t rel = now_ms - startMs;
  while (nextStep < kStepCount && rel >= kSteps[nextStep].at_ms) {
    apply(kSteps[nextStep], now_ms);
    nextStep++;
  }
  if (now_ms - lastFeedMs >= config().timing.gnss_poll_ms) {
    lastFeedMs = now_ms;
    feed_gps_records();
  }
}

bool sim_is_done() {
  return done;
}
