#pragma once
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stddef.h>

struct JsonOut {
  char *buf;
  size_t cap;
  size_t len;
  bool ok;
  bool first;

  JsonOut(char *b, size_t c) : buf(b), cap(c), len(0), ok(c > 2), first(true) {
    if (c > 0) b[0] = '\0';
    raw("{");
  }

  void raw(const char *fmt, ...) __attribute__((format(printf, 2, 3))) {
    if (!ok) return;
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(buf + len, cap - len, fmt, ap);
    va_end(ap);
    if (n < 0 || (size_t)n >= cap - len) ok = false;
    else len += (size_t)n;
  }

  void key(const char *k) {
    raw(first ? "\"%s\":" : ",\"%s\":", k);
    first = false;
  }

  void str(const char *k, const char *v) {
    key(k);
    if (v == 0) {
      raw("null");
      return;
    }
    raw("\"");
    for (; *v; ++v) {
      if (*v == '"' || *v == '\\') raw("\\%c", *v);
      else if ((unsigned char)*v < 0x20) raw("?");
      else raw("%c", *v);
    }
    raw("\"");
  }

  void u(const char *k, bool valid, unsigned long v) {
    key(k);
    if (valid) raw("%lu", v);
    else raw("null");
  }

  void i(const char *k, bool valid, long v) {
    key(k);
    if (valid) raw("%ld", v);
    else raw("null");
  }

  void num(const char *k, bool valid, double v, int dec) {
    key(k);
    if (valid) raw("%.*f", dec, v);
    else raw("null");
  }

  void flag(const char *k, bool v) {
    key(k);
    raw(v ? "true" : "false");
  }

  size_t finish() {
    raw("}");
    if (!ok) {
      if (cap > 0) buf[0] = '\0';
      return 0;
    }
    return len;
  }
};
