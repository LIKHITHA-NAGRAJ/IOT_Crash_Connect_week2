#include <string.h>
#include "msg_queue.h"

MsgQueue::MsgQueue() : head_(0), count_(0), cap_(1), policy_(OVERFLOW_DROP_NEWEST), droppedNewest_(0), droppedOldest_(0) {}

void MsgQueue::init(uint32_t capacity, OverflowPolicy policy) {
  cap_ = capacity < 1 ? 1 : (capacity > (uint32_t)MAX_QUEUE_CAP ? (uint32_t)MAX_QUEUE_CAP : capacity);
  policy_ = policy;
  head_ = 0;
  count_ = 0;
  droppedNewest_ = 0;
  droppedOldest_ = 0;
}

PushResult MsgQueue::push(const MsgItem &item, MsgItem *dropped) {
  PushResult r = PUSH_OK;
  if (count_ >= cap_) {
    if (policy_ == OVERFLOW_DROP_NEWEST) {
      droppedNewest_++;
      if (dropped) *dropped = item;
      return PUSH_DROPPED_NEWEST;
    }
    if (dropped) *dropped = items_[head_];
    head_ = (head_ + 1) % MAX_QUEUE_CAP;
    count_--;
    droppedOldest_++;
    r = PUSH_DROPPED_OLDEST;
  }
  items_[(head_ + count_) % MAX_QUEUE_CAP] = item;
  count_++;
  return r;
}

MsgItem *MsgQueue::head() {
  return count_ ? &items_[head_] : 0;
}

bool MsgQueue::pop() {
  if (!count_) return false;
  head_ = (head_ + 1) % MAX_QUEUE_CAP;
  count_--;
  return true;
}
