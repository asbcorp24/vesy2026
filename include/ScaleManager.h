#pragma once

#include <HX711.h>

#include "Types.h"

class ScaleManager {
 public:
  void begin(const AppSettings& settings);
  void updateSettings(const AppSettings& settings);
  SensorReadings read(const AppSettings& settings);
  void tare();
  void tare(uint8_t index);
  bool calibrate(uint8_t index, float referenceWeight, float& calibrationFactor);
  bool isReady(uint8_t index) const;

 private:
  void updateDerivedValues(SensorReadings& readings, const AppSettings& settings);

  HX711 scales_[4];
  bool ready_[4] = {false, false, false, false};
};
