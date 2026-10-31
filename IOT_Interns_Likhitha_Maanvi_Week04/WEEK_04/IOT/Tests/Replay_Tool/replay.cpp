#include <Arduino.h>
#include <stdio.h>
#include <string.h>
#include "Accident_Detection/detector.h"
#include "IMU/imu.h"

unsigned long fake_ms = 0;
SerialClass Serial;

struct Manifest {
  char name[64];
  char expected[16];
  char expectedType[32];
  char desc[200];
};

static int read_manifest(const char *path, Manifest *out, int max) {
  FILE *f = fopen(path, "r");
  if (!f) return 0;
  char line[400];
  int n = 0;
  fgets(line, sizeof(line), f);
  while (n < max && fgets(line, sizeof(line), f)) {
    char *parts[4] = {0};
    char *p = line;
    for (int i = 0; i < 4; ++i) {
      parts[i] = p;
      char *c = i < 3 ? strchr(p, ',') : strpbrk(p, "\r\n");
      if (!c) break;
      *c = '\0';
      p = c + 1;
    }
    if (!parts[3]) continue;
    snprintf(out[n].name, sizeof(out[n].name), "%s", parts[0]);
    snprintf(out[n].expected, sizeof(out[n].expected), "%s", parts[1]);
    snprintf(out[n].expectedType, sizeof(out[n].expectedType), "%s", parts[2]);
    snprintf(out[n].desc, sizeof(out[n].desc), "%s", parts[3]);
    n++;
  }
  fclose(f);
  return n;
}

int main(int argc, char **argv) {
  if (argc < 3) {
    printf("usage: replay <scenario_dir> <results_csv>\n");
    return 2;
  }
  char path[512];
  snprintf(path, sizeof(path), "%s/manifest.csv", argv[1]);
  static Manifest m[32];
  int count = read_manifest(path, m, 32);
  if (count == 0) {
    printf("manifest not found: %s\n", path);
    return 2;
  }
  FILE *out = fopen(argv[2], "w");
  fprintf(out, "scenario,input_type,samples,invalid_rows,expected_decision,decision,accident_type,candidates,confirmed,rejected,peak_g,max_rotation_deg,speed_check,time_to_decision_ms,reason,result,description\n");
  printf("%-24s %-9s %-10s %-9s %-4s %s\n", "scenario", "expected", "decision", "peak_g", "trig", "result");
  int pass = 0, fail = 0;
  for (int i = 0; i < count; ++i) {
    snprintf(path, sizeof(path), "%s/%s.csv", argv[1], m[i].name);
    FILE *f = fopen(path, "r");
    if (!f) {
      printf("missing %s\n", path);
      continue;
    }
    detector_init(detector_default_config());
    RingBuffer<ImuSample, 64> ring;
    ImuFilter filt;
    imu_filter_init(&filt, 0.3f);
    char line[200];
    int samples = 0, invalid = 0, cand = 0, conf = 0, rej = 0;
    DetectResult lastDecision;
    memset(&lastDecision, 0, sizeof(lastDecision));
    snprintf(lastDecision.speed_check, sizeof(lastDecision.speed_check), "-");
    while (fgets(line, sizeof(line), f)) {
      ReplayRow r;
      if (!imu_csv_parse(line, &r)) {
        if (line[0] >= '0' && line[0] <= '9') {
          invalid++;
          ImuSample bad;
          memset(&bad, 0, sizeof(bad));
          detector_step(bad, false, 0);
        }
        continue;
      }
      samples++;
      ring.push(r.imu);
      imu_filter_step(&filt, r.imu);
      DetectResult d = detector_step(r.imu, r.speed_valid, r.speed_kmh);
      if (d.decision == DET_CANDIDATE) cand++;
      if (d.decision == DET_CONFIRMED) { conf++; lastDecision = d; }
      if (d.decision == DET_REJECTED) { rej++; lastDecision = d; }
    }
    fclose(f);
    const char *decision = conf ? "CONFIRMED" : (rej ? "REJECTED" : "NONE");
    bool ok = strcmp(decision, m[i].expected) == 0;
    if (ok && conf && m[i].expectedType[0]) ok = strcmp(lastDecision.accident_type, m[i].expectedType) == 0;
    ok ? pass++ : fail++;
    fprintf(out, "%s,SIMULATED_SYNTHETIC,%d,%d,%s,%s,%s,%d,%d,%d,%.2f,%.0f,%s,%lu,%s,%s,\"%s\"\n", m[i].name, samples, invalid,
            m[i].expected, decision, conf ? lastDecision.accident_type : "", cand, conf, rej, lastDecision.peak_g,
            lastDecision.max_rotation_deg, lastDecision.speed_check, (unsigned long)lastDecision.decision_after_ms,
            lastDecision.reason, ok ? "PASS" : "FAIL", m[i].desc);
    printf("%-24s %-9s %-10s %-9.2f %-4d %s%s\n", m[i].name, m[i].expected, decision, lastDecision.peak_g, cand,
           ok ? "PASS" : "FAIL", ok ? "" : "  <-- see Known issues");
  }
  fclose(out);
  printf("\nsummary: %d scenarios, %d as expected, %d not as expected\n", pass + fail, pass, fail);
  return 0;
}
