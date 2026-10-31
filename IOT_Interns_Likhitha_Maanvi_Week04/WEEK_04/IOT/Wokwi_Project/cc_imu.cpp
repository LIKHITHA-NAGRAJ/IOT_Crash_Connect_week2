#include <Arduino.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cc_imu.h"

namespace {
ImuSample simSample;
bool simSet = false;
bool inited = false;
}

float imu_magnitude(float ax, float ay, float az) {
  return sqrtf(ax * ax + ay * ay + az * az);
}

bool imu_init() {
  inited = true;
  simSet = false;
  memset(&simSample, 0, sizeof(simSample));
  return true;
}

void imu_sim_set(const ImuSample &s) {
  simSample = s;
  simSample.mag_g = imu_magnitude(s.ax, s.ay, s.az);
  simSet = true;
}

bool imu_read(ImuSample *out) {
  memset(out, 0, sizeof(*out));
  if (!inited || !simSet) return false;
  *out = simSample;
  return out->valid;
}

void imu_filter_init(ImuFilter *f, float alpha) {
  f->alpha = alpha;
  f->smooth_mag = 1.0f;
  f->primed = false;
}

float imu_filter_step(ImuFilter *f, const ImuSample &s) {
  if (!s.valid) return f->smooth_mag;
  if (!f->primed) {
    f->smooth_mag = s.mag_g;
    f->primed = true;
  } else {
    f->smooth_mag = f->alpha * s.mag_g + (1.0f - f->alpha) * f->smooth_mag;
  }
  return f->smooth_mag;
}

float imu_motion_stddev(const RingBuffer<ImuSample, 64> &rb, size_t window) {
  size_t n = rb.size() < window ? rb.size() : window;
  if (n < 5) return -1.0f;
  double sum = 0, sq = 0;
  for (size_t i = rb.size() - n; i < rb.size(); ++i) {
    double m = rb.at(i).mag_g;
    sum += m;
    sq += m * m;
  }
  double mean = sum / (double)n;
  double var = sq / (double)n - mean * mean;
  return var > 0 ? (float)sqrt(var) : 0.0f;
}

size_t imu_csv_header(char *out, size_t cap) {
  int n = snprintf(out, cap, "t_ms,ax_g,ay_g,az_g,gx_dps,gy_dps,gz_dps,gps_speed_kmh,gps_fix");
  return n < 0 ? 0 : (size_t)n;
}

size_t imu_csv_format(const ReplayRow &r, char *out, size_t cap) {
  char spd[16] = "";
  if (r.speed_valid) snprintf(spd, sizeof(spd), "%.1f", r.speed_kmh);
  int n = snprintf(out, cap, "%lu,%.3f,%.3f,%.3f,%.1f,%.1f,%.1f,%s,%d", (unsigned long)r.imu.t_ms, r.imu.ax,
                   r.imu.ay, r.imu.az, r.imu.gx, r.imu.gy, r.imu.gz, spd, r.gps_fix ? 1 : 0);
  return n < 0 ? 0 : (size_t)n;
}

bool imu_csv_parse(const char *line, ReplayRow *out) {
  memset(out, 0, sizeof(*out));
  while (*line == ' ') ++line;
  if (*line == '\0' || *line == '#' || *line == '\n' || *line == '\r') return false;
  if ((*line < '0' || *line > '9') && *line != '-') return false;
  char tmp[160];
  snprintf(tmp, sizeof(tmp), "%s", line);
  char *fields[9] = {0};
  int n = 0;
  char *p = tmp;
  while (n < 9) {
    fields[n++] = p;
    char *c = strchr(p, ',');
    if (!c) break;
    *c = '\0';
    p = c + 1;
  }
  if (n < 9) return false;
  char *end = 0;
  out->imu.t_ms = (uint32_t)strtoul(fields[0], &end, 10);
  if (end == fields[0]) return false;
  float v[6];
  for (int i = 0; i < 6; ++i) {
    v[i] = strtof(fields[i + 1], &end);
    if (end == fields[i + 1] || isnan(v[i])) return false;
  }
  out->imu.ax = v[0]; out->imu.ay = v[1]; out->imu.az = v[2];
  out->imu.gx = v[3]; out->imu.gy = v[4]; out->imu.gz = v[5];
  out->imu.mag_g = imu_magnitude(v[0], v[1], v[2]);
  out->imu.valid = true;
  out->gps_fix = atoi(fields[8]) != 0;
  if (fields[7][0] != '\0' && out->gps_fix) {
    out->speed_valid = true;
    out->speed_kmh = strtof(fields[7], 0);
  }
  return true;
}
