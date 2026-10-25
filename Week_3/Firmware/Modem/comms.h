#pragma once

namespace Comms {
  bool begin();
  bool connectNetwork();
  bool sendData(const char* payload);
  bool isConnected();
}