#pragma once

namespace IMU {
  struct Reading {
    float ax;
    float ay;
    float az;
    float gx;
    float gy;
    float gz;
  };

  bool begin();
  Reading readData();
  float accelerationMagnitude(const Reading& reading);
}