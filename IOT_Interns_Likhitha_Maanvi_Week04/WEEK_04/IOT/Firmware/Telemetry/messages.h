#pragma once
#include <stdint.h>
#include "../Common/cc_types.h"
#include "../GNSS/gnss.h"
#include "../IMU/imu.h"
#include "../Accident_Detection/detector.h"
#include "../SOS_Events/msg_queue.h"

#define FW_VERSION "0.4.0"
#define SCHEMA_VERSION "1.0"

struct MsgContext {
  uint32_t now_ms;
  DeviceState state;
  const GnssState *gnss;
  const ImuSample *imu;
  uint32_t fault_flags;
};

void messages_boot_reset();
bool build_telemetry(const MsgContext &c, MsgItem *out);
bool build_status(const MsgContext &c, MsgItem *out);
bool build_event(EventKind kind, const DetectResult *det, const MsgContext &c, MsgItem *out);

enum MotionState : uint8_t {
  MOTION_UNKNOWN = 0,
  MOTION_STATIONARY,
  MOTION_MOVING
};

MotionState motion_update(MotionState prev, const GnssState &g, const RingBuffer<ImuSample, 64> &rb);
uint32_t report_interval_ms(MotionState m, bool event_active);
const char *motion_name(MotionState m);
