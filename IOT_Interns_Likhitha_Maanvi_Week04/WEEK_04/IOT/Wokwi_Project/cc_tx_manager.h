#pragma once
#include <stdint.h>
#include "cc_msg_queue.h"

struct TxStats {
  uint32_t enqueued;
  uint32_t attempts;
  uint32_t acked;
  uint32_t retries;
  uint32_t rejected_dropped;
  uint32_t dropped_newest;
  uint32_t dropped_oldest;
};

void tx_init();
void tx_enqueue(const MsgItem &item);
void tx_update(uint32_t now_ms);
const TxStats &tx_stats();
MsgQueue &tx_events();
MsgQueue &tx_telemetry();
uint32_t tx_backoff_s();
