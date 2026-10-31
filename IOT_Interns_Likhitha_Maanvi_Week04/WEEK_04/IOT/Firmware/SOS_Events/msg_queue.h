#pragma once
#include <stdint.h>
#include "../Config/config.h"

enum ItemKind : uint8_t {
  ITEM_TELEMETRY = 0,
  ITEM_STATUS,
  ITEM_EVENT
};

enum OverflowPolicy : uint8_t {
  OVERFLOW_DROP_NEWEST = 0,
  OVERFLOW_DROP_OLDEST
};

struct MsgItem {
  char id[64];
  ItemKind kind;
  uint16_t len;
  uint16_t attempts;
  char payload[640];
};

enum PushResult : uint8_t {
  PUSH_OK = 0,
  PUSH_DROPPED_NEWEST,
  PUSH_DROPPED_OLDEST
};

class MsgQueue {
 public:
  MsgQueue();
  void init(uint32_t capacity, OverflowPolicy policy);
  PushResult push(const MsgItem &item, MsgItem *dropped);
  MsgItem *head();
  bool pop();
  uint32_t size() const { return count_; }
  uint32_t capacity() const { return cap_; }
  uint32_t dropped_newest() const { return droppedNewest_; }
  uint32_t dropped_oldest() const { return droppedOldest_; }
  const MsgItem &at(uint32_t i) const { return items_[(head_ + i) % MAX_QUEUE_CAP]; }
 private:
  MsgItem items_[MAX_QUEUE_CAP];
  uint32_t head_;
  uint32_t count_;
  uint32_t cap_;
  OverflowPolicy policy_;
  uint32_t droppedNewest_;
  uint32_t droppedOldest_;
};
