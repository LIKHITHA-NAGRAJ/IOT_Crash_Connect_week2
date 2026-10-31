#include <string.h>
#include "cc_tx_manager.h"
#include "cc_log.h"
#include "cc_config.h"
#include "cc_modem.h"

namespace {

enum TxState { TX_IDLE, TX_WAIT, TX_BACKOFF };

MsgQueue evtQ;
MsgQueue telQ;
TxStats stats;
TxState st = TX_IDLE;
MsgQueue *cur = 0;
uint32_t waitStart = 0;
uint32_t retryAt = 0;
uint32_t failCount = 0;
uint32_t curBackoff = 0;
bool noNetLogged = false;

void on_failure(uint32_t now, ErrCode code, const char *why, const MsgItem *it) {
  failCount++;
  uint32_t b = config().backoff_init_s;
  for (uint32_t i = 1; i < failCount && b < config().backoff_max_s; ++i) b *= 2;
  if (b > config().backoff_max_s) b = config().backoff_max_s;
  curBackoff = b;
  retryAt = now + b * 1000u;
  stats.retries++;
  LOGW("TX", code, "%s id=%s attempt=%u, retry in %lus", why, it->id, (unsigned)it->attempts, (unsigned long)b);
  st = TX_BACKOFF;
}

}

void tx_init() {
  evtQ.init(config().q_evt_cap, OVERFLOW_DROP_OLDEST);
  telQ.init(config().q_tel_cap, OVERFLOW_DROP_NEWEST);
  memset(&stats, 0, sizeof(stats));
  st = TX_IDLE;
  cur = 0;
  failCount = 0;
  curBackoff = 0;
  noNetLogged = false;
}

MsgQueue &tx_events() { return evtQ; }
MsgQueue &tx_telemetry() { return telQ; }
const TxStats &tx_stats() { return stats; }
uint32_t tx_backoff_s() { return curBackoff; }

void tx_enqueue(const MsgItem &item) {
  MsgQueue &q = item.kind == ITEM_TELEMETRY ? telQ : evtQ;
  MsgItem dropped;
  PushResult r = q.push(item, &dropped);
  stats.enqueued++;
  if (r == PUSH_DROPPED_NEWEST) {
    stats.dropped_newest++;
    LOGW("QUEUE", ERR_QUEUE_DROP_NEWEST, "full (%lu), dropped incoming id=%s", (unsigned long)q.capacity(), dropped.id);
  } else if (r == PUSH_DROPPED_OLDEST) {
    stats.dropped_oldest++;
    LOGW("QUEUE", ERR_QUEUE_DROP_OLDEST, "full (%lu), dropped oldest id=%s", (unsigned long)q.capacity(), dropped.id);
  }
}

void tx_update(uint32_t now) {
  if (st == TX_WAIT) {
    int status = 0;
    PostResult r = modem_post_poll(&status);
    MsgItem *it = cur->head();
    if (r == POST_PENDING) {
      if ((now - waitStart) >= config().ack_timeout_ms + timing().modem_http_timeout_ms) {
        modem_post_abort();
        on_failure(now, ERR_TX_ACK_TIMEOUT, "ack timeout", it);
      }
      return;
    }
    if (r == POST_ACKED) {
      stats.acked++;
      failCount = 0;
      curBackoff = 0;
      LOGI("TX", "ACK http=%d id=%s (attempts=%u)", status, it->id, (unsigned)it->attempts);
      cur->pop();
      st = TX_IDLE;
    } else if (r == POST_HTTP_ERROR && (status == 400 || status == 413 || status == 422)) {
      stats.rejected_dropped++;
      LOGE("TX", ERR_TX_REJECTED, "http=%d id=%s dropped", status, it->id);
      cur->pop();
      st = TX_IDLE;
    } else if (r == POST_HTTP_ERROR) {
      on_failure(now, ERR_TX_NO_SERVER, "server error", it);
    } else {
      on_failure(now, ERR_TX_NO_SERVER, "no server answer", it);
    }
    return;
  }

  if (st == TX_BACKOFF) {
    if (now < retryAt) return;
    st = TX_IDLE;
  }

  MsgQueue *q = evtQ.size() ? &evtQ : (telQ.size() ? &telQ : 0);
  if (!q) return;
  if (!modem_ready()) {
    if (!noNetLogged) {
      LOGW("TX", ERR_TX_NO_NETWORK, "%lu event(s), %lu telemetry waiting", (unsigned long)evtQ.size(),
           (unsigned long)telQ.size());
      noNetLogged = true;
    }
    return;
  }
  noNetLogged = false;
  MsgItem *it = q->head();
  if (!modem_post_begin(now, it->payload, it->len)) return;
  it->attempts++;
  stats.attempts++;
  cur = q;
  waitStart = now;
  st = TX_WAIT;
  if (it->kind != ITEM_TELEMETRY) LOGI("TX", "send id=%s attempt=%u", it->id, (unsigned)it->attempts);
  else LOGD("TX", "send telemetry %s attempt=%u", it->id, (unsigned)it->attempts);
}
