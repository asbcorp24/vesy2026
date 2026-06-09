#include "ConnectivityManager.h"

#include <ArduinoJson.h>
#include <LittleFS.h>

#include "Config.h"

namespace {
ConnectivityManager* g_instance = nullptr;

String makeStateTopic(const AppSettings& settings) {
  String topic = Defaults::MQTT_TOPIC_STATE;
  if (strlen(settings.basketId) > 0) {
    topic += "/";
    topic += settings.basketId;
  }
  return topic;
}
}

ConnectivityManager::ConnectivityManager() : mqttClient_(wifiClient_), server_(80) {}

void ConnectivityManager::begin(AppSettings& settings, StatusCallback statusCallback, SettingsCallback settingsCallback, ActionCallback tareCallback) {
  statusCallback_ = statusCallback;
  settingsCallback_ = settingsCallback;
  tareCallback_ = tareCallback;

  WiFi.mode(WIFI_AP_STA);
  beginAccessPoint();
  beginStation(settings);

  if (!LittleFS.begin(true)) {
    Serial.println("LittleFS mount failed");
  }

  configureRoutes(settings);
  server_.begin();

  mqttClient_.setServer(settings.mqttHost, settings.mqttPort);

  g_instance = this;
  if (esp_now_init() == ESP_OK) {
    esp_now_register_send_cb(ConnectivityManager::onEspNowSent);
  }
}

void ConnectivityManager::loop(AppSettings& settings) {
  server_.handleClient();
  ensureMqtt(settings);
  mqttClient_.loop();
}

void ConnectivityManager::publishState(const SensorReadings& readings, const AppSettings& settings, bool forceSend) {
  if (settings.sendOnChangeOnly && !forceSend && !readings.changed) {
    return;
  }

  JsonDocument doc;
  doc["basketId"] = settings.basketId;
  doc["retailerId"] = settings.retailerId;
  doc["retailerName"] = settings.retailerName;
  doc["retailerPin"] = settings.retailerPin;
  doc["storePin"] = settings.storePin;
  doc["storeId"] = settings.storeId;
  doc["storeName"] = settings.storeName;
  doc["basketLocation"] = settings.basketLocation;
  doc["productName"] = settings.productName;
  doc["productCode"] = settings.productCode;
  doc["totalWeight"] = readings.totalWeight;
  doc["unitWeight"] = settings.unitWeight;
  doc["fullWeight"] = settings.fullWeight;
  doc["quantity"] = readings.quantity;
  doc["fullQuantity"] = readings.fullQuantity;
  doc["removedQuantity"] = readings.removedQuantity;
  doc["fillPercent"] = readings.fillPercent;
  doc["temperature"] = readings.temperature;
  doc["humidity"] = readings.humidity;
  doc["centerX"] = readings.centerX;
  doc["centerY"] = readings.centerY;
  doc["tiltX"] = readings.tiltX;
  doc["tiltY"] = readings.tiltY;
  doc["mode"] = settings.productMode == ProductMode::Mass ? "mass" : "pieces";

  char payload[768];
  const size_t length = serializeJson(doc, payload, sizeof(payload));

  if (mqttClient_.connected()) {
    const String topic = makeStateTopic(settings);
    mqttClient_.publish(topic.c_str(), reinterpret_cast<const uint8_t*>(payload), length);
  }

  if (strlen(settings.httpEndpoint) > 0) {
    sendHttpState(readings, settings);
  }

  if (settings.serverMac[0] || settings.serverMac[1] || settings.serverMac[2] || settings.serverMac[3] || settings.serverMac[4] || settings.serverMac[5]) {
    esp_now_send(settings.serverMac, reinterpret_cast<const uint8_t*>(payload), length);
  }

  lastPublished_ = readings;
}

bool ConnectivityManager::isWifiConnected() const {
  return WiFi.status() == WL_CONNECTED;
}

bool ConnectivityManager::isMqttConnected() {
  return mqttClient_.connected();
}

void ConnectivityManager::onEspNowSent(const uint8_t* macAddr, esp_now_send_status_t status) {
  (void)macAddr;
  (void)status;
}

void ConnectivityManager::beginAccessPoint() {
  WiFi.softAP(Defaults::AP_SSID, Defaults::AP_PASSWORD);
}

void ConnectivityManager::beginStation(const AppSettings& settings) {
  if (strlen(settings.wifiSsid) == 0) {
    return;
  }

  WiFi.begin(settings.wifiSsid, settings.wifiPassword);
  const unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < Defaults::WIFI_CONNECT_TIMEOUT_MS) {
    delay(250);
  }
}

void ConnectivityManager::configureRoutes(AppSettings& settings) {
  server_.on("/", HTTP_GET, [this]() { handleRoot(); });
  server_.on("/api/status", HTTP_GET, [this]() { handleStatus(); });
  server_.on("/api/settings", HTTP_GET, [this, &settings]() { handleSettingsGet(settings); });
  server_.on("/api/settings", HTTP_POST, [this, &settings]() { handleSettingsUpdate(settings); });
  server_.on("/api/profile", HTTP_POST, [this, &settings]() { handleWeightProfile(settings); });
  server_.on("/api/calibration", HTTP_POST, [this, &settings]() { handleCalibration(settings); });
  server_.on("/api/send", HTTP_POST, [this, &settings]() { handleSendNow(settings); });
  server_.on("/api/tare", HTTP_POST, [this]() { handleTare(); });
}

void ConnectivityManager::handleRoot() {
  if (!LittleFS.exists("/index.html")) {
    server_.send(500, "text/plain; charset=utf-8", "index.html not found in LittleFS");
    return;
  }

  File file = LittleFS.open("/index.html", "r");
  server_.streamFile(file, "text/html; charset=utf-8");
  file.close();
}

void ConnectivityManager::handleStatus() {
  const SensorReadings readings = statusCallback_ ? statusCallback_() : SensorReadings{};

  JsonDocument doc;
  doc["totalWeight"] = readings.totalWeight;
  doc["quantity"] = readings.quantity;
  doc["fullQuantity"] = readings.fullQuantity;
  doc["removedQuantity"] = readings.removedQuantity;
  doc["fillPercent"] = readings.fillPercent;
  doc["temperature"] = readings.temperature;
  doc["humidity"] = readings.humidity;
  doc["centerX"] = readings.centerX;
  doc["centerY"] = readings.centerY;
  doc["tiltX"] = readings.tiltX;
  doc["tiltY"] = readings.tiltY;
  JsonArray corners = doc["corners"].to<JsonArray>();
  for (float weight : readings.cornerWeights) {
    corners.add(weight);
  }

  String response;
  serializeJson(doc, response);
  server_.send(200, "application/json", response);
}

void ConnectivityManager::handleSettingsGet(const AppSettings& settings) {
  JsonDocument doc;
  doc["basketId"] = settings.basketId;
  doc["retailerId"] = settings.retailerId;
  doc["retailerName"] = settings.retailerName;
  doc["retailerPin"] = settings.retailerPin;
  doc["storePin"] = settings.storePin;
  doc["storeId"] = settings.storeId;
  doc["storeName"] = settings.storeName;
  doc["basketLocation"] = settings.basketLocation;
  doc["productName"] = settings.productName;
  doc["productCode"] = settings.productCode;
  doc["registrationServerUrl"] = settings.registrationServerUrl;
  doc["wifiSsid"] = settings.wifiSsid;
  doc["mqttHost"] = settings.mqttHost;
  doc["mqttPort"] = settings.mqttPort;
  doc["httpEndpoint"] = settings.httpEndpoint;
  doc["sendOnChangeOnly"] = settings.sendOnChangeOnly;
  doc["changeThreshold"] = settings.changeThreshold;
  doc["unitWeight"] = settings.unitWeight;
  doc["fullWeight"] = settings.fullWeight;
  doc["mode"] = settings.productMode == ProductMode::Mass ? "mass" : "pieces";
  doc["calibrationReferenceWeight"] = settings.calibrationReferenceWeight;
  JsonArray calibration = doc["calibration"].to<JsonArray>();
  for (float value : settings.calibration) {
    calibration.add(value);
  }

  String response;
  serializeJson(doc, response);
  server_.send(200, "application/json", response);
}

void ConnectivityManager::handleSettingsUpdate(AppSettings& settings) {
  JsonDocument doc;
  const DeserializationError error = deserializeJson(doc, server_.arg("plain"));
  if (error) {
    server_.send(400, "application/json", "{\"error\":\"invalid json\"}");
    return;
  }

  strlcpy(settings.basketId, doc["basketId"] | settings.basketId, sizeof(settings.basketId));
  strlcpy(settings.retailerId, doc["retailerId"] | settings.retailerId, sizeof(settings.retailerId));
  strlcpy(settings.retailerName, doc["retailerName"] | settings.retailerName, sizeof(settings.retailerName));
  strlcpy(settings.retailerPin, doc["retailerPin"] | settings.retailerPin, sizeof(settings.retailerPin));
  strlcpy(settings.storePin, doc["storePin"] | settings.storePin, sizeof(settings.storePin));
  strlcpy(settings.storeId, doc["storeId"] | settings.storeId, sizeof(settings.storeId));
  strlcpy(settings.storeName, doc["storeName"] | settings.storeName, sizeof(settings.storeName));
  strlcpy(settings.basketLocation, doc["basketLocation"] | settings.basketLocation, sizeof(settings.basketLocation));
  strlcpy(settings.productName, doc["productName"] | settings.productName, sizeof(settings.productName));
  strlcpy(settings.productCode, doc["productCode"] | settings.productCode, sizeof(settings.productCode));
  strlcpy(settings.registrationServerUrl, doc["registrationServerUrl"] | settings.registrationServerUrl, sizeof(settings.registrationServerUrl));
  strlcpy(settings.wifiSsid, doc["wifiSsid"] | settings.wifiSsid, sizeof(settings.wifiSsid));
  strlcpy(settings.wifiPassword, doc["wifiPassword"] | settings.wifiPassword, sizeof(settings.wifiPassword));
  strlcpy(settings.mqttHost, doc["mqttHost"] | settings.mqttHost, sizeof(settings.mqttHost));
  settings.mqttPort = doc["mqttPort"] | settings.mqttPort;
  strlcpy(settings.httpEndpoint, doc["httpEndpoint"] | settings.httpEndpoint, sizeof(settings.httpEndpoint));
  settings.sendOnChangeOnly = doc["sendOnChangeOnly"] | settings.sendOnChangeOnly;
  settings.changeThreshold = doc["changeThreshold"] | settings.changeThreshold;
  if (doc["mode"].is<const char*>()) {
    const String mode = doc["mode"].as<String>();
    settings.productMode = mode == "pieces" ? ProductMode::Pieces : ProductMode::Mass;
  }

  if (doc["serverMac"].is<const char*>()) {
    parseMacString(doc["serverMac"].as<String>(), settings.serverMac);
  }

  mqttClient_.setServer(settings.mqttHost, settings.mqttPort);
  settingsCallback_(settings);
  server_.send(200, "application/json", "{\"ok\":true}");
}

void ConnectivityManager::handleWeightProfile(AppSettings& settings) {
  JsonDocument doc;
  const DeserializationError error = deserializeJson(doc, server_.arg("plain"));
  if (error) {
    server_.send(400, "application/json", "{\"error\":\"invalid json\"}");
    return;
  }

  const SensorReadings readings = statusCallback_ ? statusCallback_() : SensorReadings{};

  if ((doc["captureUnitWeight"] | false)) {
    if (!isfinite(readings.totalWeight) || readings.totalWeight <= 1.0F) {
      server_.send(400, "application/json", "{\"error\":\"current weight is too low for unit capture\"}");
      return;
    }
    settings.unitWeight = readings.totalWeight;
  } else {
    settings.unitWeight = doc["unitWeight"] | settings.unitWeight;
  }

  if ((doc["captureFullWeight"] | false)) {
    if (!isfinite(readings.totalWeight) || readings.totalWeight <= 1.0F) {
      server_.send(400, "application/json", "{\"error\":\"current weight is too low for full capture\"}");
      return;
    }
    settings.fullWeight = readings.totalWeight;
  } else {
    settings.fullWeight = doc["fullWeight"] | settings.fullWeight;
  }

  settings.calibrationReferenceWeight = doc["calibrationReferenceWeight"] | settings.calibrationReferenceWeight;
  strlcpy(settings.productName, doc["productName"] | settings.productName, sizeof(settings.productName));
  strlcpy(settings.productCode, doc["productCode"] | settings.productCode, sizeof(settings.productCode));
  const String mode = doc["mode"] | String(settings.productMode == ProductMode::Mass ? "mass" : "pieces");
  settings.productMode = mode == "pieces" ? ProductMode::Pieces : ProductMode::Mass;

  settingsCallback_(settings);

  JsonDocument response;
  response["ok"] = true;
  response["unitWeight"] = settings.unitWeight;
  response["fullWeight"] = settings.fullWeight;
  String body;
  serializeJson(response, body);
  server_.send(200, "application/json", body);
}

void ConnectivityManager::handleCalibration(AppSettings& settings) {
  JsonDocument doc;
  const DeserializationError error = deserializeJson(doc, server_.arg("plain"));
  if (error) {
    server_.send(400, "application/json", "{\"error\":\"invalid json\"}");
    return;
  }

  JsonArray values = doc["calibration"].as<JsonArray>();
  if (values.size() == 4) {
    for (uint8_t i = 0; i < 4; ++i) {
      settings.calibration[i] = values[i].as<float>();
    }
    settingsCallback_(settings);
  }

  server_.send(200, "application/json", "{\"ok\":true}");
}

void ConnectivityManager::handleSendNow(AppSettings& settings) {
  if (statusCallback_) {
    publishState(statusCallback_(), settings, true);
  }
  server_.send(200, "application/json", "{\"ok\":true}");
}

void ConnectivityManager::handleTare() {
  if (tareCallback_) {
    tareCallback_();
  }
  server_.send(200, "application/json", "{\"ok\":true}");
}

void ConnectivityManager::ensureMqtt(const AppSettings& settings) {
  if (mqttClient_.connected() || strlen(settings.mqttHost) == 0) {
    return;
  }

  if (millis() - lastMqttAttemptMs_ < Defaults::MQTT_RECONNECT_MS) {
    return;
  }

  lastMqttAttemptMs_ = millis();
  String clientId = "smartbasket-";
  clientId += strlen(settings.basketId) > 0 ? settings.basketId : "node";
  mqttClient_.connect(clientId.c_str());
}

void ConnectivityManager::sendHttpState(const SensorReadings& readings, const AppSettings& settings) {
  HTTPClient client;
  if (!client.begin(settings.httpEndpoint)) {
    return;
  }

  JsonDocument doc;
  doc["basketId"] = settings.basketId;
  doc["retailerId"] = settings.retailerId;
  doc["retailerName"] = settings.retailerName;
  doc["retailerPin"] = settings.retailerPin;
  doc["storePin"] = settings.storePin;
  doc["storeId"] = settings.storeId;
  doc["storeName"] = settings.storeName;
  doc["basketLocation"] = settings.basketLocation;
  doc["productName"] = settings.productName;
  doc["productCode"] = settings.productCode;
  doc["totalWeight"] = readings.totalWeight;
  doc["unitWeight"] = settings.unitWeight;
  doc["fullWeight"] = settings.fullWeight;
  doc["quantity"] = readings.quantity;
  doc["fullQuantity"] = readings.fullQuantity;
  doc["removedQuantity"] = readings.removedQuantity;
  doc["fillPercent"] = readings.fillPercent;
  doc["temperature"] = readings.temperature;
  doc["humidity"] = readings.humidity;
  doc["centerX"] = readings.centerX;
  doc["centerY"] = readings.centerY;
  doc["tiltX"] = readings.tiltX;
  doc["tiltY"] = readings.tiltY;

  String body;
  serializeJson(doc, body);
  client.addHeader("Content-Type", "application/json");
  client.POST(body);
  client.end();
}

bool ConnectivityManager::parseMacString(const String& value, uint8_t* output) {
  unsigned int bytes[6];
  if (sscanf(value.c_str(), "%x:%x:%x:%x:%x:%x", &bytes[0], &bytes[1], &bytes[2], &bytes[3], &bytes[4], &bytes[5]) != 6) {
    return false;
  }

  for (uint8_t i = 0; i < 6; ++i) {
    output[i] = static_cast<uint8_t>(bytes[i]);
  }
  return true;
}
