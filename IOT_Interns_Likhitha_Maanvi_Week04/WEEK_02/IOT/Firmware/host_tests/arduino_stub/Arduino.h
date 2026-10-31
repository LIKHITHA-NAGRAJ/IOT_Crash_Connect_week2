#pragma once
#define ARDUINO 100
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdarg.h>

typedef uint8_t byte;

#define PI 3.1415926535897932384626433832795
#define TWO_PI 6.283185307179586476925286766559
#define DEG_TO_RAD 0.017453292519943295769236907684886
#define RAD_TO_DEG 57.295779513082320876798154814105
#define radians(deg) ((deg) * DEG_TO_RAD)
#define degrees(rad) ((rad) * RAD_TO_DEG)
#define sq(x) ((x) * (x))

extern unsigned long fake_ms;
inline unsigned long millis() { return fake_ms; }
inline void delay(unsigned long ms) { fake_ms += ms; }

struct SerialClass {
  void begin(int) {}
  void print(const char *s) { printf("%s", s); }
  void println(const char *s) { printf("%s\n", s); }
  void println() { printf("\n"); }
  size_t printf(const char *fmt, ...) __attribute__((format(printf, 2, 3))) {
    va_list ap;
    va_start(ap, fmt);
    int n = vprintf(fmt, ap);
    va_end(ap);
    return n < 0 ? 0 : (size_t)n;
  }
};
extern SerialClass Serial;
