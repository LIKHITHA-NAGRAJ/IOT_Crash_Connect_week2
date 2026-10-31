#pragma once
#include "../common/cc_types.h"
#include "../imu/imu.h"

EventType safety_evaluate(const ImuSample &imu, bool sos_pressed);
