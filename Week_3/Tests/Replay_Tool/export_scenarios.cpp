// Host tool: writes the SIMULATED accident-detection scenarios to CSV files.
// Build (from this folder):
//   g++ -std=c++17 -I ../../Firmware/Accident_Detection export_scenarios.cpp ../../Firmware/Accident_Detection/sim_scenarios.cpp -o export_scenarios
// Run:
//   ./export_scenarios ../../Test_Data/Accident_Scenarios
//
// CSV columns: t_ms,ax_g,ay_g,az_g,gx_dps,gy_dps,gz_dps,speed_kmh
//   speed_kmh: a number = valid GPS speed update; NA = GPS unavailable; empty = no GPS update at this row
// ALL DATA IS SIMULATED. It is not from a real sensor or vehicle.
#include <stdio.h>
#include <string.h>
#include "sim_scenarios.h"

static void onSample(const SimSample& s, void* p) {
  FILE* f = (FILE*)p;
  if (s.speedState == 1)      fprintf(f, "%u,%.4f,%.4f,%.4f,%.3f,%.3f,%.3f,%.1f\n", s.tMs, s.ax, s.ay, s.az, s.gx, s.gy, s.gz, s.speedKmh);
  else if (s.speedState == 2) fprintf(f, "%u,%.4f,%.4f,%.4f,%.3f,%.3f,%.3f,NA\n", s.tMs, s.ax, s.ay, s.az, s.gx, s.gy, s.gz);
  else                        fprintf(f, "%u,%.4f,%.4f,%.4f,%.3f,%.3f,%.3f,\n", s.tMs, s.ax, s.ay, s.az, s.gx, s.gy, s.gz);
}

int main(int argc, char** argv) {
  const char* dir = (argc > 1) ? argv[1] : ".";
  for (int i = 0; i < scenarioCount(); i++) {
    char path[512];
    snprintf(path, sizeof path, "%s/%02d_%s.csv", dir, i, scenarioInfo(i).name);
    FILE* f = fopen(path, "w");
    if (!f) { fprintf(stderr, "cannot write %s\n", path); return 1; }
    fprintf(f, "# SIMULATED DATA - not from a real sensor or vehicle. %s\n", scenarioInfo(i).description);
    fprintf(f, "t_ms,ax_g,ay_g,az_g,gx_dps,gy_dps,gz_dps,speed_kmh\n");
    generateScenario(i, onSample, f);
    fclose(f);
    printf("wrote %s\n", path);
  }
  return 0;
}
