#include "config.h"

// All values are placeholders. Not tested on hardware.
const DeviceConfig CONFIG = {
  /* deviceId  */ "CC-IOT-01",
  /* simulate  */ true,
  /* intervals */ { 1000, 100, 5000, 2000 },
  /* comms     */ { "PENDING", "PENDING", 0, 3, 2000 },
  /* event     */ { 2.0f },
  /* timeouts  */ { 60000, 30000, 3000, 3000 },
  /* pins      */ { -1, -1, -1, -1, -1, -1 },
  /* gnss      */ { 4, 5.0f },  // PLACEHOLDERS, PENDING CONFIRMATION
  /* modem     */ { 1000, 15000, 10 },  // PLACEHOLDERS, PENDING CONFIRMATION
  /* accident  */ {
    /* impact g        */ 3.0f,
    /* impact min ms   */ 20,
    /* jerk on         */ false,
    /* jerk g/s        */ 120.0f,
    /* gravity tau ms  */ 1000,
    /* quiet band g    */ 0.15f,
    /* gyro spike dps  */ 250.0f,
    /* orient deg      */ 45.0f,
    /* orient window   */ 1000,
    /* require orient  */ false,
    /* settle ms       */ 1000,
    /* still band g    */ 0.08f,
    /* still gyro dps  */ 10.0f,
    /* still min ms    */ 2000,
    /* min pre speed   */ 15.0f,
    /* speed drop      */ 15.0f,
    /* post speed max  */ 5.0f,
    /* confirm window  */ 6000,
    /* cooldown ms     */ 2000
  },  // ALL PLACEHOLDERS, PENDING CONFIRMATION
  /* sos       */ { 30, 2500, 5000 },                 // debounce ms, long press ms, cooldown ms (PLACEHOLDERS)
  /* events    */ { "1.0", false, 600, 2000, 60000 }  // PLACEHOLDERS, PENDING CONFIRMATION
};