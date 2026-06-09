#pragma once

#include <DHT.h>

class ClimateManager {
 public:
  ClimateManager();
  void begin();
  void poll(float& temperature, float& humidity);

 private:
  DHT dht_;
};
