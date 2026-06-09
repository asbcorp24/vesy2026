#include "ClimateManager.h"

#include "Config.h"

ClimateManager::ClimateManager() : dht_(Pins::DHT_PIN, DHT11) {}

void ClimateManager::begin() {
  dht_.begin();
}

void ClimateManager::poll(float& temperature, float& humidity) {
  const float currentHumidity = dht_.readHumidity();
  const float currentTemperature = dht_.readTemperature();
  if (!isnan(currentTemperature)) {
    temperature = currentTemperature;
  }
  if (!isnan(currentHumidity)) {
    humidity = currentHumidity;
  }
}
