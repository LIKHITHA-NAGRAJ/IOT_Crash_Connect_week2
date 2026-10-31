#pragma once

enum ImuCase {
  IMU_CASE_STATIONARY = 0,
  IMU_CASE_TILT,
  IMU_CASE_MOVEMENT,
  IMU_CASE_IMPACT,
  IMU_CASE_COUNT
};

struct ImuSample {
  bool valid;
  bool simulated;
  float ax, ay, az;
  float gx, gy, gz;
  float accel_mag_g;
};

bool imu_init();
bool imu_read(ImuSample *out);
bool imu_hw_read_lsm6dsox(ImuSample *out);
float imu_accel_magnitude(float ax, float ay, float az);
void imu_sim_set_case(ImuCase c);
const char *imu_case_name(ImuCase c);
bool imu_module_ok();
