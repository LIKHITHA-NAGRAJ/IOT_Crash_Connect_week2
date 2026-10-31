#pragma once
#include <stddef.h>
#include <stdint.h>

struct ImuSample {
  uint32_t t_ms;
  bool valid;
  float ax, ay, az;
  float gx, gy, gz;
  float mag_g;
};

struct ReplayRow {
  ImuSample imu;
  bool gps_fix;
  bool speed_valid;
  float speed_kmh;
};

template <typename T, size_t N>
class RingBuffer {
 public:
  RingBuffer() : head_(0), count_(0), overwritten_(0) {}
  void push(const T &v) {
    if (count_ == N) {
      head_ = (head_ + 1) % N;
      overwritten_++;
      count_--;
    }
    items_[(head_ + count_) % N] = v;
    count_++;
  }
  size_t size() const { return count_; }
  size_t capacity() const { return N; }
  const T &at(size_t i) const { return items_[(head_ + i) % N]; }
  uint32_t overwritten() const { return overwritten_; }
  void clear() { head_ = 0; count_ = 0; }
 private:
  T items_[N];
  size_t head_;
  size_t count_;
  uint32_t overwritten_;
};

float imu_magnitude(float ax, float ay, float az);
bool imu_init();
bool imu_read(ImuSample *out);
void imu_sim_set(const ImuSample &s);

struct ImuFilter {
  float alpha;
  float smooth_mag;
  bool primed;
};

void imu_filter_init(ImuFilter *f, float alpha);
float imu_filter_step(ImuFilter *f, const ImuSample &s);
float imu_motion_stddev(const RingBuffer<ImuSample, 64> &rb, size_t window);

size_t imu_csv_header(char *out, size_t cap);
size_t imu_csv_format(const ReplayRow &r, char *out, size_t cap);
bool imu_csv_parse(const char *line, ReplayRow *out);
