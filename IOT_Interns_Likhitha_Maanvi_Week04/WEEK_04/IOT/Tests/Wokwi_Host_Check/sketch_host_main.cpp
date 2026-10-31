#include <Arduino.h>
#include <stdio.h>

unsigned long fake_ms = 0;
SerialClass Serial;

void setup();
void loop();
bool sketch_finished();

int main() {
  setvbuf(stdout, 0, _IOLBF, 0);
  fake_ms = 600;
  setup();
  while (!sketch_finished()) {
    loop();
    fake_ms += 20;
  }
  return 0;
}
