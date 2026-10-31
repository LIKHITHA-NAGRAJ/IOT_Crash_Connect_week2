#include "arduino_stub/Arduino.h"

unsigned long fake_ms = 0;
SerialClass Serial;

#include "../crashconnect_fw/crashconnect_fw.ino"

int main() {
  setup();
  unsigned long extra = 0;
  while (extra < 6000) {
    fake_ms += 100;
    loop();
    if (sim_is_done()) extra += 100;
  }
  return 0;
}
