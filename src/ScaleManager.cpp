#include "ScaleManager.h"

#include "Config.h"

void ScaleManager::begin(const AppSettings& settings) {
  for (uint8_t index = 0; index < 4; ++index) {
    scales_[index].begin(Pins::HX711_DOUT[index], Pins::HX711_SCK[index]);
    ready_[index] = scales_[index].is_ready();
    if (ready_[index]) {
      scales_[index].set_scale(settings.calibration[index]);
      scales_[index].tare();
    }
  }
}

void ScaleManager::updateSettings(const AppSettings& settings) {
  for (uint8_t index = 0; index < 4; ++index) {
    if (ready_[index]) {
      scales_[index].set_scale(settings.calibration[index]);
    }
  }
}

SensorReadings ScaleManager::read(const AppSettings& settings) {
  SensorReadings readings;
  for (uint8_t index = 0; index < 4; ++index) {
    if (ready_[index] && scales_[index].is_ready()) {
      readings.cornerWeights[index] = scales_[index].get_units(2);
      readings.totalWeight += readings.cornerWeights[index];
    }
  }

  readings.rawWeight = readings.totalWeight;
  updateDerivedValues(readings, settings);
  return readings;
}

void ScaleManager::finalizeReadings(SensorReadings& readings, const AppSettings& settings) {
  updateDerivedValues(readings, settings);
}

void ScaleManager::tare() {
  for (uint8_t index = 0; index < 4; ++index) {
    if (ready_[index]) {
      scales_[index].tare();
    }
  }
}

void ScaleManager::tare(uint8_t index) {
  if (index < 4 && ready_[index]) {
    scales_[index].tare();
  }
}

bool ScaleManager::calibrate(uint8_t index, float referenceWeight, float& calibrationFactor) {
  if (index >= 4 || !ready_[index] || referenceWeight <= 0.0F) {
    return false;
  }

  const float rawAverage = scales_[index].get_value(10);
  if (!isfinite(rawAverage) || fabsf(rawAverage) < 1.0F) {
    return false;
  }

  calibrationFactor = rawAverage / referenceWeight;
  return isfinite(calibrationFactor) && fabsf(calibrationFactor) > 0.0001F;
}

bool ScaleManager::isReady(uint8_t index) const {
  return index < 4 && ready_[index];
}

void ScaleManager::updateDerivedValues(SensorReadings& readings, const AppSettings& settings) {
  const float balanceWeight = fabs(readings.rawWeight) >= 0.001F ? readings.rawWeight : readings.totalWeight;
  const float safeWeight = fabs(balanceWeight) < 0.001F ? 1.0F : balanceWeight;
  const float left = readings.cornerWeights[0] + readings.cornerWeights[2];
  const float right = readings.cornerWeights[1] + readings.cornerWeights[3];
  const float top = readings.cornerWeights[0] + readings.cornerWeights[1];
  const float bottom = readings.cornerWeights[2] + readings.cornerWeights[3];

  readings.centerX = (right - left) / safeWeight;
  readings.centerY = (top - bottom) / safeWeight;
  if (!readings.imuReady) {
    readings.tiltX = readings.centerX * 10.0F;
    readings.tiltY = readings.centerY * 10.0F;
  }

  if (settings.productMode == ProductMode::Pieces && settings.unitWeight > 0.01F) {
    readings.quantity = static_cast<uint32_t>(roundf(max(0.0F, readings.totalWeight) / settings.unitWeight));
    if (settings.fullWeight > 0.01F) {
      readings.fullQuantity = static_cast<uint32_t>(roundf(settings.fullWeight / settings.unitWeight));
      readings.removedQuantity = readings.fullQuantity > readings.quantity ? readings.fullQuantity - readings.quantity : 0;
      readings.fillPercent = min(100.0F, max(0.0F, (readings.totalWeight / settings.fullWeight) * 100.0F));
    } else {
      readings.fullQuantity = readings.quantity;
      readings.removedQuantity = 0;
      readings.fillPercent = 0.0F;
    }
  } else {
    readings.quantity = 0;
    readings.fullQuantity = 0;
    readings.removedQuantity = 0;
    readings.fillPercent = settings.fullWeight > 0.01F ? min(100.0F, max(0.0F, (readings.totalWeight / settings.fullWeight) * 100.0F)) : 0.0F;
  }
}
