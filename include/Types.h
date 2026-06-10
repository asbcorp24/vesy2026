#pragma once

#include <Arduino.h>

enum class ProductMode : uint8_t {
  Mass = 0,
  Pieces = 1,
};

struct SensorReadings {
  float cornerWeights[4] = {0, 0, 0, 0};
  float rawWeight = 0.0F;
  float totalWeight = 0.0F;
  float compensationFactor = 1.0F;
  float centerX = 0.0F;
  float centerY = 0.0F;
  float tiltX = 0.0F;
  float tiltY = 0.0F;
  bool imuReady = false;
  float temperature = NAN;
  float humidity = NAN;
  uint32_t quantity = 0;
  uint32_t fullQuantity = 0;
  uint32_t removedQuantity = 0;
  float fillPercent = 0.0F;
  bool changed = false;
};

struct AppSettings {
  char basketId[24] = "basket-01";
  char retailerId[24] = "";
  char retailerName[48] = "";
  char retailerPin[16] = "";
  char storePin[16] = "";
  char storeId[24] = "";
  char storeName[48] = "";
  char basketLocation[48] = "";
  char productName[48] = "";
  char productCode[32] = "";
  char registrationServerUrl[96] = "";
  char wifiSsid[32] = "";
  char wifiPassword[64] = "";
  char mqttHost[64] = "";
  uint16_t mqttPort = 1883;
  char httpEndpoint[96] = "";
  ProductMode productMode = ProductMode::Mass;
  float unitWeight = 100.0F;
  float fullWeight = 0.0F;
  float calibrationReferenceWeight = 500.0F;
  float calibration[4] = {1.0F, 1.0F, 1.0F, 1.0F};
  float tiltZeroX = 0.0F;
  float tiltZeroY = 0.0F;
  bool sendOnChangeOnly = true;
  float changeThreshold = 5.0F;
  uint8_t serverMac[6] = {0, 0, 0, 0, 0, 0};
};
