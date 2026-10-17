#include "config.h"

// All values are placeholders. Not tested on hardware.
const DeviceConfig CONFIG = {
  /* deviceId  */ "CC-IOT-01",
  /* simulate  */ true,
  /* intervals */ { 1000, 100, 5000, 2000 },
  /* comms     */ { "PENDING", "PENDING", 0, 3, 2000 },
  /* event     */ { 2.0f },
  /* timeouts  */ { 60000, 30000, 3000, 3000 },
  /* pins      */ { -1, -1, -1, -1, -1, -1 }
};