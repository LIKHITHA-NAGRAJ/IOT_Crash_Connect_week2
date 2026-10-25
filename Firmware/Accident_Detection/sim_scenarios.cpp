#include "sim_scenarios.h"

// SIMULATED data only. See sim_scenarios.h.

static const ScenarioInfo INFO[] = {
  // name,                 description,                                                         ideal, cand, conf, candJerk
  { "normal_cruise",       "steady driving with road vibration",                                 false, 0, 0, 0 },
  { "gentle_accel_stop",   "gentle acceleration, cruise, soft stop",                             false, 0, 0, 0 },
  { "hard_braking",        "0.8 g braking from 60 km/h to a stop, no impact",                    false, 0, 0, 0 },
  { "cornering_handling",  "sustained 0.6 g and 0.8 g lateral turns",                            false, 0, 0, 0 },
  { "pothole_single",      "one 4 g sample (20 ms) while driving",                               false, 0, 0, 1 },
  { "pothole_double_hit",  "two 3.6 g samples (40 ms) while driving, driving continues",         false, 1, 0, 1 },
  { "bump_then_hard_stop", "bump, then a hard stop to 0 km/h (known false-positive risk)",       false, 1, 1, 1 },
  { "parked_door_slam",    "3.4 g knock while parked",                                           false, 1, 0, 1 },
  { "minor_contact_drive", "5 g contact at 30 km/h, then driving continues",                     false, 1, 0, 1 },
  { "frontal_impact_stop", "controlled-impact simulation: 9 g, stop within 0.5 s, then still",   true,  1, 1, 1 },
  { "rollover_like",       "6 g impact, 400 deg/s tumble, comes to rest on its side",            true,  1, 1, 1 },
  { "impact_gps_missing",  "frontal impact but GPS speed unavailable",                           true,  1, 1, 1 },
  { "bump_gps_missing_rough","double bump, rough road, GPS unavailable",                         false, 1, 0, 1 },
  { "bump_gps_missing_smooth","double bump, very smooth road, GPS unavailable (false-positive risk)", false, 1, 1, 1 },
};
static const int N = sizeof(INFO) / sizeof(INFO[0]);

int scenarioCount() { return N; }
const ScenarioInfo& scenarioInfo(int i) { return INFO[i]; }

namespace {
  const float MOVING_NOISE_G = 0.12f;   // assumed road vibration while driving (SIMULATED)
  const float MOVING_NOISE_DPS = 3.0f;
  const float STILL_NOISE_G = 0.01f;    // assumed sensor noise at rest (SIMULATED)
  const float STILL_NOISE_DPS = 0.5f;
  const float SMOOTH_NOISE_G = 0.03f;

  struct Gen {
    void (*cb)(const SimSample&, void*);
    void* ctx;
    uint32_t t;
    uint32_t rng;
    bool gps;

    float rnd() {                       // deterministic uniform value in [-1, 1]
      rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5;
      return ((rng & 0xFFFF) / 32767.5f) - 1.0f;
    }
    void sample(float ax, float ay, float az, float gx, float gy, float gz,
                float spd, float nG, float nD) {
      SimSample s;
      s.tMs = t;
      s.ax = ax + nG * rnd(); s.ay = ay + nG * rnd(); s.az = az + nG * rnd();
      s.gx = gx + nD * rnd(); s.gy = gy + nD * rnd(); s.gz = gz + nD * rnd();
      if (t % 1000 == 0) { s.speedState = gps ? 1 : 2; s.speedKmh = spd; }
      else               { s.speedState = 0;          s.speedKmh = 0; }
      cb(s, ctx);
      t += 20;
    }
    void seg(float sec, float ax, float ay, float az, float gx, float gy, float gz,
             float s0, float s1, float nG, float nD) {
      int n = (int)(sec * 50.0f);
      for (int i = 0; i < n; i++) {
        float f = (n > 1) ? (float)i / (float)(n - 1) : 0.0f;
        sample(ax, ay, az, gx, gy, gz, s0 + (s1 - s0) * f, nG, nD);
      }
    }
    void cruise(float sec, float kmh, float nG = MOVING_NOISE_G) {
      seg(sec, 0, 0, 1, 0, 0, 0, kmh, kmh, nG, MOVING_NOISE_DPS);
    }
    void rest(float sec) { seg(sec, 0, 0, 1, 0, 0, 0, 0, 0, STILL_NOISE_G, STILL_NOISE_DPS); }
    void hit(int samples, float ax, float ay, float az, float gx, float gy, float gz, float spd) {
      for (int i = 0; i < samples; i++) sample(ax, ay, az, gx, gy, gz, spd, 0.05f, 5.0f);
    }
  };
}

void generateScenario(int idx, void (*cb)(const SimSample&, void*), void* ctx) {
  Gen g; g.cb = cb; g.ctx = ctx; g.t = 0; g.rng = 2463534242u + (uint32_t)idx * 7919u; g.gps = true;
  switch (idx) {
    case 0:  // normal_cruise
      g.cruise(30, 50);
      break;
    case 1:  // gentle_accel_stop
      g.seg(5, 0.2f, 0, 1, 0, 0, 0, 0, 30, MOVING_NOISE_G, MOVING_NOISE_DPS);
      g.cruise(10, 30);
      g.seg(6, -0.4f, 0, 1, 0, 0, 0, 30, 0, MOVING_NOISE_G, MOVING_NOISE_DPS);
      g.rest(6);
      break;
    case 2:  // hard_braking
      g.cruise(10, 60);
      g.seg(3, -0.8f, 0, 1, 0, 0, 0, 60, 0, MOVING_NOISE_G, MOVING_NOISE_DPS);
      g.rest(8);
      break;
    case 3:  // cornering_handling
      g.cruise(10, 40);
      g.seg(4, 0, 0.6f, 1, 0, 0, 30, 40, 40, MOVING_NOISE_G, MOVING_NOISE_DPS);
      g.cruise(3, 40);
      g.seg(2, 0, 0.8f, 1, 0, 0, 45, 40, 40, MOVING_NOISE_G, MOVING_NOISE_DPS);
      g.cruise(8, 40);
      break;
    case 4:  // pothole_single
      g.cruise(10, 40);
      g.hit(1, 0.1f, 0.1f, 4.0f, 20, 5, 5, 40);
      g.cruise(8, 40);
      break;
    case 5:  // pothole_double_hit
      g.cruise(10, 40);
      g.hit(2, 0.1f, 0.1f, 3.6f, 20, 5, 5, 40);
      g.cruise(10, 40);
      break;
    case 6:  // bump_then_hard_stop
      g.cruise(10, 40);
      g.hit(2, 0.1f, 0.1f, 3.6f, 20, 5, 5, 40);
      g.cruise(1, 40);
      g.seg(1.6f, -0.9f, 0, 1, 0, 0, 0, 40, 0, MOVING_NOISE_G, MOVING_NOISE_DPS);
      g.rest(8);
      break;
    case 7:  // parked_door_slam
      g.rest(10);
      g.hit(2, 0.3f, 0.2f, 3.4f, 60, 20, 10, 0);
      g.rest(10);
      break;
    case 8:  // minor_contact_drive
      g.cruise(10, 30);
      g.hit(3, -5.0f, 0.5f, 1.2f, 15, 10, 5, 30);
      g.cruise(10, 30);
      break;
    case 9:  // frontal_impact_stop
      g.cruise(15, 50);
      g.hit(4, -9.0f, 0.3f, 1.2f, 20, 10, 5, 50);
      g.seg(0.5f, -2.0f, 0, 1, 5, 5, 5, 45, 0, 0.2f, 10);
      g.rest(10);
      break;
    case 10: // rollover_like
      g.cruise(15, 50);
      g.hit(4, -2.0f, 6.0f, 1.0f, 150, 0, 0, 50);
      g.seg(1.2f, 0, 0.8f, 0.3f, 400, 50, 30, 50, 0, 0.5f, 40);
      g.seg(10, 0, 1.0f, 0.0f, 0, 0, 0, 0, 0, STILL_NOISE_G, STILL_NOISE_DPS);   // resting on its side
      break;
    case 11: // impact_gps_missing
      g.gps = false;
      g.cruise(15, 50);
      g.hit(4, -9.0f, 0.3f, 1.2f, 20, 10, 5, 50);
      g.seg(0.5f, -2.0f, 0, 1, 5, 5, 5, 45, 0, 0.2f, 10);
      g.rest(10);
      break;
    case 12: // bump_gps_missing_rough
      g.gps = false;
      g.cruise(10, 40);
      g.hit(2, 0.1f, 0.1f, 3.6f, 20, 5, 5, 40);
      g.cruise(10, 40);
      break;
    case 13: // bump_gps_missing_smooth
      g.gps = false;
      g.cruise(10, 40, SMOOTH_NOISE_G);
      g.hit(2, 0.1f, 0.1f, 3.6f, 20, 5, 5, 40);
      g.cruise(10, 40, SMOOTH_NOISE_G);
      break;
  }
}
