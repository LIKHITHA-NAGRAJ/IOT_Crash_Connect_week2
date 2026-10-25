#include "telemetry.h"
#include "config.h"
#include <stdio.h>
#include <stdarg.h>

namespace Telemetry {

  // Appends formatted text at output[len]. Returns false if it does not fit.
  static bool put(char* output, size_t size, size_t& len, const char* fmt, ...) {
    if (len >= size) return false;
    va_list args;
    va_start(args, fmt);
    int n = vsnprintf(output + len, size - len, fmt, args);
    va_end(args);
    if (n < 0 || (size_t)n >= size - len) return false;
    len += (size_t)n;
    return true;
  }

  bool buildJson(const Data& data, char* output, size_t outputSize) {
    if (output == nullptr || outputSize == 0) return false;
    size_t len = 0;
    output[0] = '\0';
    bool ok = true;

    ok = ok && put(output, outputSize, len, "{\"device_id\":\"%s\",", CONFIG.deviceId);

    // timestamp: quoted string, or null if unavailable
    if (data.timestampUtc && data.timestampUtc[0] != '\0')
      ok = ok && put(output, outputSize, len, "\"timestamp_utc\":\"%s\",", data.timestampUtc);
    else
      ok = ok && put(output, outputSize, len, "\"timestamp_utc\":null,");

    // location block: independent of the IMU block
    if (data.locationValid)
      ok = ok && put(output, outputSize, len,
        "\"location\":{\"valid\":true,\"latitude\":%.6f,\"longitude\":%.6f},",
        data.latitude, data.longitude);
    else
      ok = ok && put(output, outputSize, len,
        "\"location\":{\"valid\":false,\"latitude\":null,\"longitude\":null},");

    // IMU block: independent of the location block
    if (data.imuAvailable)
      ok = ok && put(output, outputSize, len,
        "\"imu\":{\"available\":true,\"accel_x_g\":%.3f,\"accel_y_g\":%.3f,\"accel_z_g\":%.3f,"
        "\"gyro_x_dps\":%.3f,\"gyro_y_dps\":%.3f,\"gyro_z_dps\":%.3f},",
        data.imu.ax, data.imu.ay, data.imu.az, data.imu.gx, data.imu.gy, data.imu.gz);
    else
      ok = ok && put(output, outputSize, len,
        "\"imu\":{\"available\":false,\"accel_x_g\":null,\"accel_y_g\":null,\"accel_z_g\":null,"
        "\"gyro_x_dps\":null,\"gyro_y_dps\":null,\"gyro_z_dps\":null},");

    // "simulated" is an extra field added for labelling. PENDING CONFIRMATION.
    ok = ok && put(output, outputSize, len,
      "\"event_active\":%s,\"device_state\":\"%s\",\"simulated\":%s}",
      data.eventActive ? "true" : "false",
      DeviceState::toString(DeviceState::get()),
      CONFIG.simulateHardware ? "true" : "false");

    if (!ok) { output[0] = '\0'; return false; }
    return true;
  }
}
