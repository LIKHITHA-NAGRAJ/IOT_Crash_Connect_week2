// Host tool: replays a CSV of IMU samples (and optional GPS speed) through the accident detector.
// Build (from this folder):
//   g++ -std=c++17 -I ../../Firmware -I ../../Firmware/Accident_Detection replay_csv.cpp ../../Firmware/Accident_Detection/accident_detector.cpp ../../Firmware/config.cpp -o replay_csv
// Run:
//   ./replay_csv ../../Test_Data/Accident_Scenarios/09_frontal_impact_stop.csv          (default configuration)
//   ./replay_csv file.csv --jerk                                    (also trigger on jerk)
//   ./replay_csv file.csv --impact-g 4.0 --window-ms 8000           (override parameters)
//
// CSV columns: t_ms,ax_g,ay_g,az_g,gx_dps,gy_dps,gz_dps,speed_kmh
//   lines starting with # and the header line are skipped
//   speed_kmh: number = valid GPS speed update; NA = GPS unavailable; empty = no GPS update at this row
// Placeholder thresholds are used. Results on simulated or logged data are NOT validated accident detection.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "accident_detector.h"
#include "config.h"

int main(int argc, char** argv) {
  if (argc < 2) { fprintf(stderr, "usage: %s file.csv [--jerk] [--impact-g X] [--window-ms N] [--require-orientation]\n", argv[0]); return 1; }
  AccidentConfig cfg = CONFIG.accident;
  for (int i = 2; i < argc; i++) {
    if (!strcmp(argv[i], "--jerk")) cfg.jerkEnabled = true;
    else if (!strcmp(argv[i], "--require-orientation")) cfg.requireOrientation = true;
    else if (!strcmp(argv[i], "--impact-g") && i + 1 < argc) cfg.impactThresholdG = (float)atof(argv[++i]);
    else if (!strcmp(argv[i], "--window-ms") && i + 1 < argc) cfg.confirmWindowMs = (uint32_t)atol(argv[++i]);
  }
  FILE* f = fopen(argv[1], "r");
  if (!f) { fprintf(stderr, "cannot open %s\n", argv[1]); return 1; }

  AccidentDetector det;
  det.begin(cfg);
  char line[256];
  unsigned long rows = 0;
  printf("Replaying %s (placeholder thresholds; impact %.1f g, window %u ms, jerk %s)\n",
         argv[1], cfg.impactThresholdG, (unsigned)cfg.confirmWindowMs, cfg.jerkEnabled ? "on" : "off");
  while (fgets(line, sizeof line, f)) {
    if (line[0] == '#' || line[0] == 't') continue;
    unsigned t; float ax, ay, az, gx, gy, gz; char sp[32] = "";
    int n = sscanf(line, "%u,%f,%f,%f,%f,%f,%f,%31[^,\r\n]", &t, &ax, &ay, &az, &gx, &gy, &gz, sp);
    if (n < 7) continue;
    if (n == 8 && sp[0] != '\0') {
      if (!strcmp(sp, "NA")) det.feedSpeed(false, 0.0f);
      else det.feedSpeed(true, (float)atof(sp));
    }
    ImuInput in = { t, ax, ay, az, gx, gy, gz };
    AccidentResult r;
    rows++;
    if (det.feedSample(in, r)) {
      printf("  decision at %u ms (candidate %u ms%s): %s\n", r.decisionMs, r.candidateMs, r.triggeredByJerk ? ", jerk" : "", r.reason);
      printf("    peak %.1f g, gyro peak %.0f dps (spike %s), orientation %.0f deg (%s), stillness %s, speed %s%s\n",
             r.peakG, r.peakGyroDps, r.gyroSpike ? "yes" : "no", r.orientationDeg, r.orientationChanged ? "changed" : "unchanged",
             r.stillness ? "yes" : "no", r.speedDrop ? "drop" : (r.preSpeedKnown ? "no drop" : "unknown"), r.lowConfidence ? ", LOW CONFIDENCE" : "");
    }
  }
  fclose(f);
  AccidentStats s = det.stats();
  printf("Rows %lu | candidates %u | confirmed %u | rejected %u\n", rows, s.candidates, s.confirmed, s.rejected);
  return 0;
}
