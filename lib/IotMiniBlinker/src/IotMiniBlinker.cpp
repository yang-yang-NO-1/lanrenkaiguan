#include "IotMiniBlinker.h"

static const char* IOT_SETTINGS_FILE = "/iot_mini_blinker.json";

IotMiniBlinker* IotMiniBlinker::_active = nullptr;

IotMiniBlinker::IotMiniBlinker()
  : _mqtt(_plainClient) {
}

void IotMiniBlinker::copyText(char* dst, size_t dstSize, const char* src) {
  if (!dst || dstSize == 0) return;
  if (!src) src = "";
  strncpy(dst, src, dstSize - 1);
  dst[dstSize - 1] = '\0';
}

void IotMiniBlinker::config(const char* ssid,
                            const char* pass,
                            const char* mqttHost,
                            uint16_t mqttPort,
                            const char* mqttUser,
                            const char* mqttPassword,
                            const char* accountId,
                            const char* deviceId,
                            const char* deviceName,
                            const char* deviceType,
                            const char* topicBase,
                            const char* firmwareName,
                            const char* firmwareVersion,
                            bool mqttUseTls) {
  copyText(_ssid, sizeof(_ssid), ssid);
  copyText(_pass, sizeof(_pass), pass);
  copyText(_mqttHost, sizeof(_mqttHost), mqttHost);
  _mqttPort = mqttPort ? mqttPort : 1883;
  _mqttUseTls = mqttUseTls || (_mqttPort == 8883);
  copyText(_mqttUser, sizeof(_mqttUser), mqttUser);
  copyText(_mqttPassword, sizeof(_mqttPassword), mqttPassword);
  copyText(_accountId, sizeof(_accountId), accountId);
  copyText(_deviceId, sizeof(_deviceId), deviceId && strlen(deviceId) ? deviceId : "device");
  copyText(_deviceName, sizeof(_deviceName), deviceName && strlen(deviceName) ? deviceName : _deviceId);
  copyText(_deviceType, sizeof(_deviceType), deviceType && strlen(deviceType) ? deviceType : "generic");
  copyText(_topicBase, sizeof(_topicBase), topicBase && strlen(topicBase) ? topicBase : "u/home/devices");
  copyText(_firmwareName, sizeof(_firmwareName), firmwareName && strlen(firmwareName) ? firmwareName : "iot-mini-blinker");
  copyText(_firmwareVersion, sizeof(_firmwareVersion), firmwareVersion && strlen(firmwareVersion) ? firmwareVersion : "0.0.0");
}

bool IotMiniBlinker::mountFs() {
#if defined(ESP8266)
  bool ok = LittleFS.begin();
#else
  bool ok = LittleFS.begin(true);
#endif
  if (_debugEnabled) {
    Serial.println(ok ? F("[FS] LittleFS mounted") : F("[FS] LittleFS mount failed"));
  }
  return ok;
}

bool IotMiniBlinker::loadStoredSettings() {
  if (!LittleFS.exists(IOT_SETTINGS_FILE)) {
    if (_debugEnabled) Serial.println(F("[CONFIG] no stored settings, using defaults"));
    return false;
  }

  File file = LittleFS.open(IOT_SETTINGS_FILE, "r");
  if (!file) return false;

  DynamicJsonDocument doc(1024);
  DeserializationError err = deserializeJson(doc, file);
  file.close();
  if (err) {
    if (_debugEnabled) {
      Serial.print(F("[CONFIG] parse failed: "));
      Serial.println(err.c_str());
    }
    return false;
  }

  copyText(_mqttHost, sizeof(_mqttHost), doc["mqttHost"] | _mqttHost);
  _mqttPort = doc["mqttPort"] | _mqttPort;
  _mqttUseTls = doc["mqttUseTls"] | _mqttUseTls;
  copyText(_mqttUser, sizeof(_mqttUser), doc["mqttUser"] | _mqttUser);
  copyText(_mqttPassword, sizeof(_mqttPassword), doc["mqttPassword"] | _mqttPassword);
  copyText(_accountId, sizeof(_accountId), doc["accountId"] | _accountId);
  copyText(_deviceId, sizeof(_deviceId), doc["deviceId"] | _deviceId);
  copyText(_deviceName, sizeof(_deviceName), doc["deviceName"] | _deviceName);
  copyText(_deviceType, sizeof(_deviceType), doc["deviceType"] | _deviceType);
  copyText(_topicBase, sizeof(_topicBase), doc["topicBase"] | _topicBase);

  if (_debugEnabled) {
    Serial.println(F("[CONFIG] loaded stored settings"));
    Serial.print(F("  mqttHost=")); Serial.println(_mqttHost);
    Serial.print(F("  mqttPort=")); Serial.println(_mqttPort);
    Serial.print(F("  mqttUseTls=")); Serial.println(_mqttUseTls ? F("1") : F("0"));
    Serial.print(F("  topicBase=")); Serial.println(_topicBase);
    Serial.print(F("  deviceId=")); Serial.println(_deviceId);
  }
  return true;
}

bool IotMiniBlinker::saveStoredSettings() {
  File file = LittleFS.open(IOT_SETTINGS_FILE, "w");
  if (!file) return false;

  DynamicJsonDocument doc(1024);
  doc["mqttHost"] = _mqttHost;
  doc["mqttPort"] = _mqttPort;
  doc["mqttUseTls"] = _mqttUseTls;
  doc["mqttUser"] = _mqttUser;
  doc["mqttPassword"] = _mqttPassword;
  doc["accountId"] = _accountId;
  doc["deviceId"] = _deviceId;
  doc["deviceName"] = _deviceName;
  doc["deviceType"] = _deviceType;
  doc["topicBase"] = _topicBase;

  bool ok = serializeJson(doc, file) > 0;
  file.close();

  if (_debugEnabled) {
    Serial.println(ok ? F("[CONFIG] saved") : F("[CONFIG] save failed"));
  }
  return ok;
}

void IotMiniBlinker::begin() {
  _active = this;

  if (_debugEnabled) {
    Serial.println();
    Serial.println(F("[IotMiniBlinker] begin"));
    Serial.print(F("[IotMiniBlinker] deviceId="));
    Serial.println(_deviceId[0] ? _deviceId : "device");
  }

  mountFs();
  loadStoredSettings();

  if (_networkIndicatorEnabled) {
    pinMode(_networkIndicatorPin, OUTPUT);
    updateNetworkIndicator();
  }

  if (_portalButtonEnabled) {
    pinMode(_portalButtonPin, INPUT_PULLUP);
  }

  WiFi.mode(WIFI_STA);

#if IOT_ENABLE_WIFI_MANAGER
  bool forcePortal = false;
  if (_portalButtonEnabled && readPortalButton()) {
    unsigned long start = millis();
    while (readPortalButton() && millis() - start < 2600UL) {
      delay(20);
      yield();
    }
    forcePortal = (millis() - start >= 2500UL);
  }
  connectWiFiWithManager(forcePortal);
#else
  ensureWiFi();
#endif

  setupMqttClient();

  if (_debugEnabled) {
    Serial.print(F("[IotMiniBlinker] topicPrefix="));
    Serial.println(topicPrefix());
  }
}

void IotMiniBlinker::loop() {
  handlePortalButton();
  ensureWiFi();
  ensureMqtt();

  if (_mqtt.connected()) {
    _mqtt.loop();
  }

  const unsigned long now = millis();
  if (_mqtt.connected() && (now - _lastHeartbeat >= IOT_HEARTBEAT_MS || _lastHeartbeat == 0)) {
    _lastHeartbeat = now;
    publishStatus(true, "heartbeat", true);
  }

  updateNetworkIndicator();
}

void IotMiniBlinker::setDebug(bool enabled) {
  _debugEnabled = enabled;
}

void IotMiniBlinker::setNetworkIndicator(uint8_t pin, uint8_t activeLevel, bool enabled) {
  _networkIndicatorPin = pin;
  _networkIndicatorActiveLevel = activeLevel;
  _networkIndicatorEnabled = enabled;
  if (_networkIndicatorEnabled) {
    pinMode(_networkIndicatorPin, OUTPUT);
    updateNetworkIndicator();
  }
}

void IotMiniBlinker::setPortalButton(uint8_t pin, uint8_t activeLevel, unsigned long holdMs, bool enabled) {
  _portalButtonPin = pin;
  _portalButtonActiveLevel = activeLevel;
  _portalButtonHoldMs = holdMs;
  _portalButtonEnabled = enabled;
  if (_portalButtonEnabled) {
    pinMode(_portalButtonPin, INPUT_PULLUP);
  }
}

bool IotMiniBlinker::readPortalButton() const {
  if (!_portalButtonEnabled) return false;
  return digitalRead(_portalButtonPin) == _portalButtonActiveLevel;
}

void IotMiniBlinker::handlePortalButton() {
  if (!_portalButtonEnabled) return;

  bool pressed = readPortalButton();
  unsigned long now = millis();

  if (pressed != _portalButtonLastPressed && now - _portalButtonLastChangeAt > 40UL) {
    _portalButtonLastChangeAt = now;
    _portalButtonLastPressed = pressed;
    if (pressed) {
      _portalButtonPressedAt = now;
      _portalButtonOpened = false;
    }
  }

  if (pressed && !_portalButtonOpened && now - _portalButtonPressedAt >= _portalButtonHoldMs) {
    _portalButtonOpened = true;
    startConfigPortal();
  }
}

void IotMiniBlinker::updateNetworkIndicator() {
  if (!_networkIndicatorEnabled) return;
  const bool online = wifiConnected() && connected();
  const uint8_t offLevel = (_networkIndicatorActiveLevel == HIGH) ? LOW : HIGH;
  digitalWrite(_networkIndicatorPin, online ? offLevel : _networkIndicatorActiveLevel);
}

void IotMiniBlinker::setupMqttClient() {
  if (_mqttUseTls || _mqttPort == 8883) {
#if defined(ESP8266)
    _secureClient.setInsecure();
#else
    _secureClient.setInsecure();
#endif
    _mqtt.setClient(_secureClient);
    _mqttUseTls = true;
    if (_debugEnabled) Serial.println(F("[MQTT] TLS mode enabled"));
  } else {
    _mqtt.setClient(_plainClient);
    if (_debugEnabled) Serial.println(F("[MQTT] plain TCP mode enabled"));
  }

  _mqtt.setServer(_mqttHost, _mqttPort);
  _mqtt.setBufferSize(IOT_MQTT_BUFFER_SIZE);
  _mqtt.setKeepAlive(30);
  _mqtt.setSocketTimeout(4);
  _mqtt.setCallback(IotMiniBlinker::mqttCallbackStatic);
}

bool IotMiniBlinker::connectWiFiWithManager(bool forcePortal) {
#if !IOT_ENABLE_WIFI_MANAGER
  (void)forcePortal;
  return false;
#else
  WiFiManager wm;
  wm.setDebugOutput(_debugEnabled);
  wm.setConfigPortalTimeout(IOT_PORTAL_TIMEOUT_SEC);
  wm.setConnectTimeout(IOT_PORTAL_CONNECT_TIMEOUT_SEC);
  wm.setBreakAfterConfig(false);
  wm.setCustomHeadElement(IOT_PORTAL_HEAD_HTML);
  wm.setTitle(IOT_PORTAL_TITLE);

  char portBuf[8];
  snprintf(portBuf, sizeof(portBuf), "%u", _mqttPort);
  char tlsBuf[4];
  snprintf(tlsBuf, sizeof(tlsBuf), "%u", (_mqttUseTls || _mqttPort == 8883) ? 1 : 0);

  WiFiManagerParameter p_info(IOT_PORTAL_INFO_HTML);
  WiFiManagerParameter p_host("mqtt_host", "MQTT Host", _mqttHost, sizeof(_mqttHost));
  WiFiManagerParameter p_port("mqtt_port", "MQTT Port", portBuf, sizeof(portBuf));
  WiFiManagerParameter p_tls("mqtt_tls", "MQTT TLS: 1/0", tlsBuf, sizeof(tlsBuf));
  WiFiManagerParameter p_user("mqtt_user", "MQTT Username", _mqttUser, sizeof(_mqttUser));
  WiFiManagerParameter p_pass("mqtt_pass", "MQTT Password", _mqttPassword, sizeof(_mqttPassword));
  WiFiManagerParameter p_account("account_id", "Account ID", _accountId, sizeof(_accountId));
  WiFiManagerParameter p_base("topic_base", "Topic Base", _topicBase, sizeof(_topicBase));
  WiFiManagerParameter p_id("device_id", "Device ID", _deviceId, sizeof(_deviceId));
  WiFiManagerParameter p_name("device_name", "Device Name", _deviceName, sizeof(_deviceName));
  WiFiManagerParameter p_type("device_type", "Device Type", _deviceType, sizeof(_deviceType));

  wm.addParameter(&p_info);
  wm.addParameter(&p_host);
  wm.addParameter(&p_port);
  wm.addParameter(&p_tls);
  wm.addParameter(&p_user);
  wm.addParameter(&p_pass);
  wm.addParameter(&p_account);
  wm.addParameter(&p_base);
  wm.addParameter(&p_id);
  wm.addParameter(&p_name);
  wm.addParameter(&p_type);

  String id = _deviceId[0] ? _deviceId : chipText();
  String apName = String(IOT_PORTAL_AP_PREFIX) + id;

  if (_debugEnabled) {
    Serial.print(F("[WiFiManager] AP="));
    Serial.println(apName);
    Serial.print(F("[WiFiManager] Password="));
    Serial.println(IOT_PORTAL_AP_PASSWORD);
  }

  bool ok = false;
  if (forcePortal) {
    ok = wm.startConfigPortal(apName.c_str(), IOT_PORTAL_AP_PASSWORD);
  } else {
    ok = wm.autoConnect(apName.c_str(), IOT_PORTAL_AP_PASSWORD);
  }

  copyText(_mqttHost, sizeof(_mqttHost), p_host.getValue());
  _mqttPort = (uint16_t)atoi(p_port.getValue());
  if (_mqttPort == 0) _mqttPort = 1883;
  _mqttUseTls = atoi(p_tls.getValue()) != 0 || _mqttPort == 8883;
  copyText(_mqttUser, sizeof(_mqttUser), p_user.getValue());
  copyText(_mqttPassword, sizeof(_mqttPassword), p_pass.getValue());
  copyText(_accountId, sizeof(_accountId), p_account.getValue());
  copyText(_topicBase, sizeof(_topicBase), p_base.getValue());
  copyText(_deviceId, sizeof(_deviceId), p_id.getValue());
  copyText(_deviceName, sizeof(_deviceName), p_name.getValue());
  copyText(_deviceType, sizeof(_deviceType), p_type.getValue());

  if (!_topicBase[0]) copyText(_topicBase, sizeof(_topicBase), "u/home/devices");
  if (!_deviceId[0]) copyText(_deviceId, sizeof(_deviceId), "device");
  if (!_deviceName[0]) copyText(_deviceName, sizeof(_deviceName), _deviceId);
  if (!_deviceType[0]) copyText(_deviceType, sizeof(_deviceType), "generic");

  saveStoredSettings();

  if (_debugEnabled) {
    Serial.print(F("[WiFiManager] connected="));
    Serial.println(ok ? F("true") : F("false"));
    if (WiFi.status() == WL_CONNECTED) {
      Serial.print(F("[WiFi] IP="));
      Serial.println(WiFi.localIP());
    }
  }
  return ok;
#endif
}

void IotMiniBlinker::startConfigPortal() {
#if IOT_ENABLE_WIFI_MANAGER
  if (_debugEnabled) Serial.println(F("[WiFiManager] start config portal"));
  if (_mqtt.connected()) _mqtt.disconnect();
  connectWiFiWithManager(true);
  setupMqttClient();
  ensureMqtt();
#else
  if (_debugEnabled) Serial.println(F("[WiFiManager] disabled by IOT_ENABLE_WIFI_MANAGER=0"));
#endif
}

void IotMiniBlinker::resetSettings(bool restartDevice) {
  if (_debugEnabled) Serial.println(F("[IotMiniBlinker] reset settings"));
  if (_mqtt.connected()) _mqtt.disconnect();
  LittleFS.remove(IOT_SETTINGS_FILE);
#if IOT_ENABLE_WIFI_MANAGER
  WiFiManager wm;
  wm.resetSettings();
#endif
  if (restartDevice) {
    delay(300);
    ESP.restart();
  }
}

bool IotMiniBlinker::connected() {
  return _mqtt.connected();
}

bool IotMiniBlinker::wifiConnected() const {
  return WiFi.status() == WL_CONNECTED;
}

String IotMiniBlinker::topicPrefix() const {
  String base = _topicBase[0] ? _topicBase : "u/home/devices";
  String id = _deviceId[0] ? _deviceId : "device";
  if (base.endsWith("/")) base.remove(base.length() - 1);
  return base + "/" + id;
}

String IotMiniBlinker::topic(const char* subTopic) const {
  String p = topicPrefix();
  String s = subTopic ? subTopic : "";
  if (s.startsWith("/")) s.remove(0, 1);
  return p + "/" + s;
}

bool IotMiniBlinker::attach(const char* widgetId, IotMiniCallback cb) {
  return addHandler(IotMiniHandlerKind::WidgetId, widgetId, cb);
}

bool IotMiniBlinker::attachAction(const char* action, IotMiniCallback cb) {
  return addHandler(IotMiniHandlerKind::Action, action, cb);
}

void IotMiniBlinker::onAny(IotMiniCallback cb) {
  _anyCallback = cb;
}

void IotMiniBlinker::onConnected(IotMiniSimpleCallback cb) {
  _connectedCallback = cb;
}

bool IotMiniBlinker::publish(const char* subTopic, const String& payload, bool retained) {
  if (!_mqtt.connected()) return false;
  String t = topic(subTopic);
  if (_debugEnabled) {
    Serial.print(F("[MQTT OUT] "));
    Serial.print(t);
    Serial.print(F(" "));
    Serial.println(payload);
  }
  return _mqtt.publish(t.c_str(), payload.c_str(), retained);
}

bool IotMiniBlinker::publishJson(const char* subTopic, JsonDocument& doc, bool retained) {
  String payload;
  serializeJson(doc, payload);
  return publish(subTopic, payload, retained);
}

bool IotMiniBlinker::publishState(JsonDocument& doc, bool retained) {
  return publishJson("state", doc, retained);
}

bool IotMiniBlinker::publishMeta(bool retained) {
  DynamicJsonDocument doc(IOT_JSON_DOC_SIZE);
  doc["version"] = 2;
  doc["deviceId"] = _deviceId[0] ? _deviceId : "device";
  doc["name"] = _deviceName[0] ? _deviceName : "device";
  doc["type"] = _deviceType[0] ? _deviceType : "generic";
  doc["accountId"] = _accountId[0] ? _accountId : "device";
  doc["topicPrefix"] = topicPrefix();
  doc["fw"] = _firmwareName[0] ? _firmwareName : "iot-mini-blinker";
  doc["fwVersion"] = _firmwareVersion[0] ? _firmwareVersion : "0.0.0";
  doc["protocol"] = "iot-mini-blinker-mqtt";
  doc["wifiManager"] = (bool)IOT_ENABLE_WIFI_MANAGER;
  doc["portalAp"] = String(IOT_PORTAL_AP_PREFIX) + (_deviceId[0] ? _deviceId : "device");
  doc["mqttHost"] = _mqttHost;
  doc["mqttPort"] = _mqttPort;
  doc["mqttUseTls"] = _mqttUseTls;
  doc["chip"] = chipText();
  doc["ip"] = ipText();
  doc["cmdTopic"] = topic("cmd");
  doc["cmdWildcard"] = topic("cmd/#");
  doc["stateTopic"] = topic("state");
  doc["statusTopic"] = topic("status");
  doc["ackTopic"] = topic("ack");
  doc["eventTopic"] = topic("event");

  JsonArray widgets = doc.createNestedArray("widgets");
  for (uint8_t i = 0; i < IOT_MAX_HANDLERS; i++) {
    if (_handlers[i].used && _handlers[i].kind == IotMiniHandlerKind::WidgetId) {
      widgets.add(_handlers[i].key);
    }
  }

  return publishJson("meta", doc, retained);
}

bool IotMiniBlinker::publishStatus(bool online, const char* reason, bool retained) {
  DynamicJsonDocument doc(768);
  doc["deviceId"] = _deviceId[0] ? _deviceId : "device";
  doc["online"] = online;
  doc["status"] = online ? "online" : "offline";
  doc["reason"] = reason ? reason : "";
  doc["ip"] = ipText();
  doc["rssi"] = wifiConnected() ? WiFi.RSSI() : 0;
  doc["uptimeMs"] = millis();
  doc["uptime"] = (uint32_t)(millis() / 1000UL);
  doc["freeHeap"] = ESP.getFreeHeap();
  doc["ts"] = millis();
  doc["lastSeen"] = millis();
  return publishJson("status", doc, retained);
}

bool IotMiniBlinker::publishAck(const IotMiniCommand& cmd, bool ok, const char* message) {
  // v1.3: 小程序在线状态只看 status.online。
  // 每次设备能收到并处理命令时，先刷新 retained status，避免“能控制但显示离线”。
  publishStatus(true, "command_ack", true);

  DynamicJsonDocument doc(768);
  doc["deviceId"] = _deviceId[0] ? _deviceId : "device";
  doc["msgId"] = cmd.msgId;
  doc["widgetId"] = cmd.widgetId;
  doc["action"] = cmd.action;
  doc["ok"] = ok;
  doc["message"] = message ? message : "";
  doc["ts"] = millis();
  return publishJson("ack", doc, false);
}

bool IotMiniBlinker::publishEvent(const char* eventType, JsonDocument& doc, bool retained) {
  doc["deviceId"] = _deviceId[0] ? _deviceId : "device";
  doc["eventType"] = eventType ? eventType : "event";
  doc["ts"] = millis();
  return publishJson("event", doc, retained);
}

bool IotMiniBlinker::print(const char* widgetId, const char* textValue) {
  DynamicJsonDocument doc(768);
  doc["widgetId"] = widgetId ? widgetId : "print";
  doc["text"] = textValue ? textValue : "";
  return publishEvent("print", doc, false);
}

bool IotMiniBlinker::print(const char* widgetId, const String& textValue) {
  return print(widgetId, textValue.c_str());
}

bool IotMiniBlinker::number(const char* widgetId, float value, const char* unit) {
  DynamicJsonDocument doc(768);
  doc["widgetId"] = widgetId ? widgetId : "number";
  doc["value"] = value;
  if (unit) doc["unit"] = unit;
  return publishEvent("number", doc, false);
}

bool IotMiniBlinker::text(const char* widgetId, const char* line1, const char* line2) {
  DynamicJsonDocument doc(768);
  doc["widgetId"] = widgetId ? widgetId : "text";
  doc["line1"] = line1 ? line1 : "";
  if (line2) doc["line2"] = line2;
  return publishEvent("text", doc, false);
}

bool IotMiniBlinker::notify(const char* title, const char* message) {
  DynamicJsonDocument doc(768);
  doc["title"] = title ? title : "通知";
  doc["message"] = message ? message : "";
  return publishEvent("notify", doc, false);
}

bool IotMiniBlinker::push(const char* title, const char* message) {
  DynamicJsonDocument doc(768);
  doc["title"] = title ? title : "PUSH";
  doc["message"] = message ? message : "";
  return publishEvent("push", doc, false);
}

bool IotMiniBlinker::sms(const char* phoneOrTag, const char* message) {
  DynamicJsonDocument doc(768);
  doc["target"] = phoneOrTag ? phoneOrTag : "default";
  doc["message"] = message ? message : "";
  return publishEvent("sms", doc, false);
}

bool IotMiniBlinker::wechat(const char* openIdOrTag, const char* message) {
  DynamicJsonDocument doc(768);
  doc["target"] = openIdOrTag ? openIdOrTag : "default";
  doc["message"] = message ? message : "";
  return publishEvent("wechat", doc, false);
}

bool IotMiniBlinker::summary(const char* title, const char* message) {
  DynamicJsonDocument doc(768);
  doc["title"] = title ? title : "Summary";
  doc["message"] = message ? message : "";
  return publishEvent("summary", doc, false);
}

bool IotMiniBlinker::debug(const char* message) {
  if (_debugEnabled) {
    Serial.print(F("[DEBUG] "));
    Serial.println(message ? message : "");
  }
  DynamicJsonDocument doc(768);
  doc["message"] = message ? message : "";
  return publishEvent("debug", doc, false);
}

bool IotMiniBlinker::debug(const String& message) {
  return debug(message.c_str());
}

bool IotMiniBlinker::paramBool(JsonObjectConst params, const char* key, bool fallback) {
  if (params.isNull() || !key || !params.containsKey(key)) return fallback;
  return paramLooksOn(params[key], fallback);
}

int IotMiniBlinker::paramInt(JsonObjectConst params, const char* key, int fallback) {
  if (params.isNull() || !key || !params.containsKey(key)) return fallback;
  JsonVariantConst v = params[key];
  if (v.is<int>()) return v.as<int>();
  if (v.is<float>()) return (int)v.as<float>();
  if (v.is<const char*>()) return String(v.as<const char*>()).toInt();
  if (v.is<bool>()) return v.as<bool>() ? 1 : 0;
  return fallback;
}

float IotMiniBlinker::paramFloat(JsonObjectConst params, const char* key, float fallback) {
  if (params.isNull() || !key || !params.containsKey(key)) return fallback;
  JsonVariantConst v = params[key];
  if (v.is<float>() || v.is<double>() || v.is<int>()) return v.as<float>();
  if (v.is<const char*>()) return String(v.as<const char*>()).toFloat();
  if (v.is<bool>()) return v.as<bool>() ? 1.0f : 0.0f;
  return fallback;
}

String IotMiniBlinker::paramString(JsonObjectConst params, const char* key, const char* fallback) {
  if (params.isNull() || !key || !params.containsKey(key)) return String(fallback ? fallback : "");
  JsonVariantConst v = params[key];
  if (v.is<const char*>()) return String(v.as<const char*>());
  if (v.is<int>()) return String(v.as<int>());
  if (v.is<float>()) return String(v.as<float>());
  if (v.is<bool>()) return v.as<bool>() ? "true" : "false";
  return String(fallback ? fallback : "");
}

bool IotMiniBlinker::paramLooksOn(JsonVariantConst value, bool fallback) {
  if (value.isNull()) return fallback;
  if (value.is<bool>()) return value.as<bool>();
  if (value.is<int>()) return value.as<int>() != 0;
  if (value.is<float>()) return value.as<float>() != 0.0f;
  if (value.is<const char*>()) {
    String s = value.as<const char*>();
    s.trim();
    s.toLowerCase();
    if (s == "on" || s == "true" || s == "1" || s == "open" || s == "enable" || s == "enabled") return true;
    if (s == "off" || s == "false" || s == "0" || s == "close" || s == "disable" || s == "disabled") return false;
  }
  return fallback;
}

bool IotMiniBlinker::parseColorHex(const String& color, uint8_t& r, uint8_t& g, uint8_t& b) {
  String s = color;
  s.trim();
  if (s.startsWith("#")) s.remove(0, 1);
  if (s.length() != 6) return false;
  char* endPtr = nullptr;
  unsigned long value = strtoul(s.c_str(), &endPtr, 16);
  if (endPtr == s.c_str()) return false;
  r = (value >> 16) & 0xFF;
  g = (value >> 8) & 0xFF;
  b = value & 0xFF;
  return true;
}

bool IotMiniBlinker::addHandler(IotMiniHandlerKind kind, const char* key, IotMiniCallback cb) {
  if (!key || !cb) return false;

  // 同名覆盖。
  for (uint8_t i = 0; i < IOT_MAX_HANDLERS; i++) {
    if (_handlers[i].used && _handlers[i].kind == kind && _handlers[i].key == key) {
      _handlers[i].cb = cb;
      return true;
    }
  }

  for (uint8_t i = 0; i < IOT_MAX_HANDLERS; i++) {
    if (!_handlers[i].used) {
      _handlers[i].used = true;
      _handlers[i].kind = kind;
      _handlers[i].key = key;
      _handlers[i].cb = cb;
      return true;
    }
  }

  if (_debugEnabled) {
    Serial.println(F("[IotMiniBlinker] no free handler slots"));
  }
  return false;
}

void IotMiniBlinker::ensureWiFi() {
  if (WiFi.status() == WL_CONNECTED) return;

#if IOT_ENABLE_WIFI_MANAGER
  const unsigned long now = millis();
  if (_lastWifiAttempt != 0 && now - _lastWifiAttempt < IOT_WIFI_RETRY_MS) return;
  _lastWifiAttempt = now;
  if (_debugEnabled) Serial.println(F("[WiFi] disconnected, reconnecting saved credentials"));
  WiFi.reconnect();
#else
  if (!_ssid[0]) return;
  const unsigned long now = millis();
  if (_lastWifiAttempt != 0 && now - _lastWifiAttempt < IOT_WIFI_RETRY_MS) return;
  _lastWifiAttempt = now;

  if (_debugEnabled) {
    Serial.print(F("[WiFi] connecting to "));
    Serial.println(_ssid);
  }
  WiFi.disconnect();
  WiFi.mode(WIFI_STA);
  WiFi.begin(_ssid, _pass);
#endif
}

void IotMiniBlinker::ensureMqtt() {
  if (_mqtt.connected()) return;
  if (WiFi.status() != WL_CONNECTED) return;
  if (!_mqttHost[0]) return;

  const unsigned long now = millis();
  if (_lastMqttAttempt != 0 && now - _lastMqttAttempt < IOT_MQTT_RETRY_MS) return;
  _lastMqttAttempt = now;

  setupMqttClient();

  if (_debugEnabled) {
    Serial.print(F("[MQTT] connecting to "));
    Serial.print(_mqttHost);
    Serial.print(F(":"));
    Serial.print(_mqttPort);
    Serial.print(F(" tls="));
    Serial.println(_mqttUseTls ? F("1") : F("0"));
  }

  String willTopic = topic("status");
  String willPayload = String("{\"deviceId\":\"") + (_deviceId[0] ? _deviceId : "device") +
                       "\",\"online\":false,\"status\":\"offline\",\"reason\":\"lwt\"}";

  const String clientId = makeClientId();
  bool ok = false;
  if (hasMqttUser()) {
    ok = _mqtt.connect(clientId.c_str(), _mqttUser, _mqttPassword,
                       willTopic.c_str(), 0, true, willPayload.c_str());
  } else {
    ok = _mqtt.connect(clientId.c_str(), willTopic.c_str(), 0, true, willPayload.c_str());
  }

  if (ok) {
    String cmdWildcard = topic("cmd/#");
    _mqtt.subscribe(cmdWildcard.c_str());

    if (_debugEnabled) {
      Serial.print(F("[MQTT] connected, subscribed: "));
      Serial.println(cmdWildcard);
    }

    publishStatus(true, "mqtt_connected", true);
    publishMeta(true);
    if (_connectedCallback) _connectedCallback();
  } else {
    if (_debugEnabled) {
      Serial.print(F("[MQTT] failed, rc="));
      Serial.println(_mqtt.state());
    }
  }
}

void IotMiniBlinker::mqttCallbackStatic(char* topic, uint8_t* payload, unsigned int length) {
  if (_active) {
    _active->handleMqttMessage(topic, payload, length);
  }
}

void IotMiniBlinker::handleMqttMessage(char* rawTopic, uint8_t* payload, unsigned int length) {
  String body;
  body.reserve(length + 1);
  for (unsigned int i = 0; i < length; i++) {
    body += (char)payload[i];
  }

  if (_debugEnabled) {
    Serial.print(F("[MQTT IN] "));
    Serial.print(rawTopic);
    Serial.print(F(" "));
    Serial.println(body);
  }

  DynamicJsonDocument doc(IOT_JSON_DOC_SIZE);
  DeserializationError err = deserializeJson(doc, body);
  if (err) {
    if (_debugEnabled) {
      Serial.print(F("[JSON] parse error: "));
      Serial.println(err.c_str());
    }
    DynamicJsonDocument eventDoc(512);
    eventDoc["message"] = String("JSON parse error: ") + err.c_str();
    eventDoc["raw"] = body;
    publishEvent("error", eventDoc, false);
    return;
  }

  IotMiniCommand cmd;
  cmd.rawTopic = rawTopic;
  cmd.rawPayload = body;
  cmd.msgId = doc["msgId"] | "";
  cmd.action = doc["action"] | "";
  cmd.widgetId = doc["widgetId"] | "";
  cmd.receivedMs = millis();

  // 防止同一 msgId 因重放或重复订阅被执行两次。
  if (cmd.msgId.length() > 0 && cmd.msgId == _lastMsgId) {
    if (_debugEnabled) Serial.println(F("[MQTT] duplicate msgId ignored"));
    return;
  }
  if (cmd.msgId.length() > 0) _lastMsgId = cmd.msgId;

  JsonObjectConst params = doc["params"].as<JsonObjectConst>();
  routeCommand(cmd, params);
}

void IotMiniBlinker::routeCommand(const IotMiniCommand& cmd, JsonObjectConst params) {
  if (_anyCallback) {
    _anyCallback(cmd, params);
  }

  bool matched = false;

  if (cmd.widgetId.length() > 0) {
    for (uint8_t i = 0; i < IOT_MAX_HANDLERS; i++) {
      if (_handlers[i].used && _handlers[i].kind == IotMiniHandlerKind::WidgetId && _handlers[i].key == cmd.widgetId) {
        matched = true;
        _handlers[i].cb(cmd, params);
      }
    }
  }

  if (cmd.action.length() > 0) {
    for (uint8_t i = 0; i < IOT_MAX_HANDLERS; i++) {
      if (_handlers[i].used && _handlers[i].kind == IotMiniHandlerKind::Action && _handlers[i].key == cmd.action) {
        matched = true;
        _handlers[i].cb(cmd, params);
      }
    }
  }

  if (!matched) {
    if (cmd.action == "get" || cmd.action == "sync" || cmd.action == "state") {
      publishAck(cmd, true, "state request received");
    } else {
      publishAck(cmd, false, "no handler matched");
      DynamicJsonDocument eventDoc(768);
      eventDoc["message"] = "no handler matched";
      eventDoc["action"] = cmd.action;
      eventDoc["widgetId"] = cmd.widgetId;
      publishEvent("warn", eventDoc, false);
    }
  }
}

String IotMiniBlinker::makeClientId() const {
  String id = _deviceId[0] ? _deviceId : "device";
  id += "-";
  id += chipText();
  return id;
}

String IotMiniBlinker::ipText() const {
  if (!wifiConnected()) return "0.0.0.0";
  return WiFi.localIP().toString();
}

String IotMiniBlinker::chipText() const {
#if defined(ESP8266)
  return String(ESP.getChipId(), HEX);
#elif defined(ESP32)
  uint64_t mac = ESP.getEfuseMac();
  uint32_t low = (uint32_t)(mac & 0xFFFFFFFFULL);
  uint16_t high = (uint16_t)((mac >> 32) & 0xFFFF);
  String out = String(high, HEX);
  out += String(low, HEX);
  return out;
#else
  return "unknown";
#endif
}

bool IotMiniBlinker::hasMqttUser() const {
  return strlen(_mqttUser) > 0;
}
