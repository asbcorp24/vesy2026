#pragma once

#include <HTTPClient.h>
#include <PubSubClient.h>
#include <WebServer.h>
#include <WiFi.h>
#include <esp_now.h>

#include <functional>

#include "Types.h"

class ConnectivityManager {
 public:
  using StatusCallback = std::function<SensorReadings(void)>;
  using SettingsCallback = std::function<void(const AppSettings&)>;
  using ActionCallback = std::function<void(void)>;

  ConnectivityManager();

  void begin(
      AppSettings& settings,
      StatusCallback statusCallback,
      SettingsCallback settingsCallback,
      ActionCallback tareCallback,
      ActionCallback tiltZeroCallback);
  void loop(AppSettings& settings);
  void publishState(const SensorReadings& readings, const AppSettings& settings, bool forceSend);
  bool isWifiConnected() const;
  bool isMqttConnected();

 private:
  static void onEspNowSent(const uint8_t* macAddr, esp_now_send_status_t status);
  void beginAccessPoint();
  void beginStation(const AppSettings& settings);
  void configureRoutes(AppSettings& settings);
  void handleRoot();
  void handleStatus();
  void handleSettingsGet(const AppSettings& settings);
  void handleSettingsUpdate(AppSettings& settings);
  void handleWeightProfile(AppSettings& settings);
  void handleCalibration(AppSettings& settings);
  void handleSendNow(AppSettings& settings);
  void handleTare();
  void handleTiltZero();
  void ensureMqtt(const AppSettings& settings);
  void sendHttpState(const SensorReadings& readings, const AppSettings& settings);
  bool parseMacString(const String& value, uint8_t* output);

  WiFiClient wifiClient_;
  PubSubClient mqttClient_;
  WebServer server_;
  StatusCallback statusCallback_;
  SettingsCallback settingsCallback_;
  ActionCallback tareCallback_;
  ActionCallback tiltZeroCallback_;
  SensorReadings lastPublished_;
  unsigned long lastMqttAttemptMs_ = 0;
};
