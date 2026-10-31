#include <Arduino.h>
#include <math.h>
#include "sim_timeline.h"
#include "../App/app.h"
#include "../Common/log.h"
#include "../Config/config.h"
#include "../GNSS/gnss.h"
#include "../IMU/imu.h"
#include "../SOS_Events/tx_manager.h"
#include "nmea_gen.h"

namespace {

MockAtPort *port = 0;
uint32_t base = 0;
uint32_t lastGnss = 0;
bool done[8];
const double kEndS = 190.0;

ImuSample profile(double t) {
  ImuSample s;
  s.valid = true;
  s.t_ms = 0;
  double vib = (t >= 70.0 && t < 95.0) ? 0.12 : 0.01;
  s.ax = 0.02f + (float)(vib * sin(t * 13.0));
  s.ay = 0.01f + (float)(vib * cos(t * 7.0));
  s.az = 1.0f + (float)(vib * sin(t * 5.0));
  s.gx = (float)(2.0 * sin(t * 3.0));
  s.gy = (float)(1.5 * cos(t * 2.0));
  s.gz = 1.0f;
  if (t >= 95.0 && t < 95.1) {
    int k = (int)((t - 95.0) / 0.02 + 0.5);
    const float g[5] = {6.5f, 7.0f, 5.0f, 3.5f, 2.0f};
    s.ax = g[k % 5];
    s.ay = -1.2f;
    s.az = 1.0f;
  } else if (t >= 95.1 && t < 95.5) {
    s.ax = 0.8f; s.ay = 0.5f; s.az = 1.1f;
    s.gx = 150; s.gy = 90; s.gz = 60;
  }
  return s;
}

double gps_speed(double t) {
  if (t < 70.0) return 0.0;
  if (t < 96.0) return 45.0;
  if (t < 97.0) return 20.0;
  return 0.0;
}

bool gps_has_fix(double t) {
  if (t < 14.0) return false;
  if (t >= 125.0 && t < 145.0) return false;
  return true;
}

}

void sim_begin(uint32_t now, MockAtPort *p) {
  port = p;
  base = now;
  lastGnss = now;
  for (int i = 0; i < 8; ++i) done[i] = false;
  Serial.printf("CRASH CONNECT firmware v0.4 demo - ALL INPUTS SIMULATED (generated NMEA, synthetic IMU, mock modem, mock server)\n");
}

bool sim_step(uint32_t now, bool extraSos) {
  double t = (now - base) / 1000.0;
  if (t > kEndS) return false;

  if (!done[0] && t >= 50.0) {
    done[0] = true;
    bool ok = config_set_u32("rpt_still_s", 10);
    Serial.printf("---- t=50s TEST ACTION: stationary interval changed 30s -> 10s via stored config (ok=%d)\n", ok);
  }
  if (!done[1] && t >= 88.0) { done[1] = true; port->registered = false; Serial.printf("---- t=88s TEST ACTION: network registration removed (mock)\n"); }
  if (!done[2] && t >= 100.0) { done[2] = true; port->registered = true; Serial.printf("---- t=100s TEST ACTION: network registration restored (mock)\n"); }
  if (!done[3] && t >= 104.0) { done[3] = true; port->server = SERVER_DOWN; Serial.printf("---- t=104s TEST ACTION: mock server DOWN\n"); }
  if (!done[4] && t >= 122.0) { done[4] = true; port->server = SERVER_OK; Serial.printf("---- t=122s TEST ACTION: mock server UP\n"); }
  if (!done[5] && t >= 150.0) { done[5] = true; app_sim_hang(true); Serial.printf("---- t=150s TEST ACTION: forced firmware hang (watchdog test)\n"); }

  bool sos = (t >= 80.0 && t < 81.0) || (t >= 85.0 && t < 87.5) || (t >= 106.0 && t < 108.5) || (t >= 130.0 && t < 132.5);
  app_sim_set_sos(sos || extraSos);

  if (now - lastGnss >= 1000) {
    lastGnss = now;
    char buf[360];
    nmea_pair(8 * 3600 + (int)t, gps_has_fix(t), gps_speed(t), buf, sizeof(buf));
    gnss_feed(buf);
  }
  imu_sim_set(profile(t));

  if (app_sim_watchdog_check(now)) {
    Serial.printf("---- simulated reset, rebooting firmware\n");
    app_setup(now, port, true);
  }
  app_loop(now);
  return true;
}

void sim_print_summary() {
  const TxStats &st = tx_stats();
  Serial.printf("\n==== DEMO SUMMARY (SIMULATED) ====\n");
  Serial.printf("telemetry built=%lu status built=%lu accident events=%lu sos events=%lu\n",
                (unsigned long)app_stats().telemetry_built, (unsigned long)app_stats().status_built,
                (unsigned long)app_stats().accident_events, (unsigned long)app_stats().sos_events);
  Serial.printf("after last boot: enqueued=%lu attempts=%lu acked=%lu retries=%lu rejected=%lu dropped_newest=%lu dropped_oldest=%lu\n",
                (unsigned long)st.enqueued, (unsigned long)st.attempts, (unsigned long)st.acked, (unsigned long)st.retries,
                (unsigned long)st.rejected_dropped, (unsigned long)st.dropped_newest, (unsigned long)st.dropped_oldest);
  Serial.printf("mock server received %lu posts, watchdog resets=%lu\n", (unsigned long)port->received_count,
                (unsigned long)app_stats().resets);
  Serial.printf("mock server log (order of arrival):\n");
  for (uint32_t i = 0; i < port->received_count && i < 48; ++i)
    Serial.printf("  #%02lu http=%d id=%s\n", (unsigned long)(i + 1), port->received_status[i], port->received_ids[i]);
  Serial.printf("log counts: errors=%lu warnings=%lu\n", (unsigned long)log_count(CCLOG_ERROR), (unsigned long)log_count(CCLOG_WARN));
}
