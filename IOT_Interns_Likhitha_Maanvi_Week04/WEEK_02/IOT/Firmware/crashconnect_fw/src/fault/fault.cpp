#include "fault.h"

namespace {

uint32_t flags = 0;

}

void fault_set(uint32_t bits) {
  flags |= bits;
}

void fault_clear(uint32_t bits) {
  flags &= ~bits;
}

void fault_clear_all() {
  flags = 0;
}

uint32_t fault_flags() {
  return flags;
}

bool fault_is_critical() {
  return (flags & FAULT_CRITICAL_MASK) != 0;
}

bool fault_is_noncritical() {
  return (flags & ~FAULT_CRITICAL_MASK) != 0;
}
