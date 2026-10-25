#pragma once

#include "gnss.h"
#include "imu.h"
#include "state_machine.h"
#include <stddef.h>

// Telemetry packet builder (DRAFT). Field names and format are PENDING CONFIRMATION
// until compared with the agreed common packet and the Full Stack API contract.
//
// UNAVAILABLE VALUES are written as JSON null, never as 0 or an invented number.

namespace Telemetry {
  struct Data {
    const char* timestampUtc;   // nullptr if no verified UTC time is available
    bool locationValid;         // false -> latitude/longitude are written as null
    double latitude;            // double, not float (float is too coarse for 6 decimals)
    double longitude;
    bool imuAvailable;          // false -> all IMU values are written as null
    IMU::Reading imu;
    bool eventActive;
  };

  // Returns true on success. On failure the output buffer is emptied.
  bool buildJson(const Data& data, char* output, size_t outputSize);
}
