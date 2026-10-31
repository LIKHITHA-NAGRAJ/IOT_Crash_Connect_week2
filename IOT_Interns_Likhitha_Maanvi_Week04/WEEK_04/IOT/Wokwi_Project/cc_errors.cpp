#include "cc_errors.h"

namespace {

struct Row {
  ErrCode code;
  const char *name;
  ErrSeverity sev;
  const char *text;
};

const Row kRows[] = {
#define CC_ERR_ROW(name, num, sev, text) {name, #name, sev, text},
  CC_ERROR_TABLE(CC_ERR_ROW)
#undef CC_ERR_ROW
};

const int kCount = (int)(sizeof(kRows) / sizeof(kRows[0]));

const Row *find(ErrCode c) {
  for (int i = 0; i < kCount; ++i) {
    if (kRows[i].code == c) return &kRows[i];
  }
  return 0;
}

}

const char *err_name(ErrCode c) {
  const Row *r = find(c);
  return r ? r->name : "ERR_UNKNOWN";
}

const char *err_text(ErrCode c) {
  const Row *r = find(c);
  return r ? r->text : "Unknown error code";
}

ErrSeverity err_severity(ErrCode c) {
  const Row *r = find(c);
  return r ? r->sev : SEV_ERROR;
}

const char *err_severity_name(ErrSeverity s) {
  switch (s) {
    case SEV_INFO:     return "INFO";
    case SEV_WARNING:  return "WARNING";
    case SEV_ERROR:    return "ERROR";
    default:           return "CRITICAL";
  }
}

int err_table_size() {
  return kCount;
}

bool err_table_at(int index, ErrCode *code, const char **name, ErrSeverity *sev, const char **text) {
  if (index < 0 || index >= kCount) return false;
  *code = kRows[index].code;
  *name = kRows[index].name;
  *sev = kRows[index].sev;
  *text = kRows[index].text;
  return true;
}
