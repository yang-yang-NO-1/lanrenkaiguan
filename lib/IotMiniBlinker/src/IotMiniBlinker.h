#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <WiFiManager.h>
#include <functional>

#if defined(ESP8266)
  #include <ESP8266WiFi.h>
  #include <WiFiClient.h>
  #include <WiFiClientSecureBearSSL.h>
  #include <LittleFS.h>
#elif defined(ESP32)
  #include <WiFi.h>
  #include <WiFiClient.h>
  #include <WiFiClientSecure.h>
  #include <FS.h>
  #include <LittleFS.h>
#else
  #error "IotMiniBlinker only supports ESP8266/ESP32 Arduino cores in this project."
#endif

#ifndef IOT_MAX_HANDLERS
  #define IOT_MAX_HANDLERS 48
#endif

#ifndef IOT_MQTT_BUFFER_SIZE
  #define IOT_MQTT_BUFFER_SIZE 2048
#endif

#ifndef IOT_JSON_DOC_SIZE
  #define IOT_JSON_DOC_SIZE 3072
#endif

#ifndef IOT_HEARTBEAT_MS
  #define IOT_HEARTBEAT_MS 30000UL
#endif

#ifndef IOT_WIFI_RETRY_MS
  #define IOT_WIFI_RETRY_MS 5000UL
#endif

#ifndef IOT_MQTT_RETRY_MS
  #define IOT_MQTT_RETRY_MS 3000UL
#endif

#ifndef IOT_ENABLE_WIFI_MANAGER
  #define IOT_ENABLE_WIFI_MANAGER 1
#endif

#ifndef IOT_PORTAL_TIMEOUT_SEC
  #define IOT_PORTAL_TIMEOUT_SEC 180
#endif

#ifndef IOT_PORTAL_CONNECT_TIMEOUT_SEC
  #define IOT_PORTAL_CONNECT_TIMEOUT_SEC 20
#endif

#ifndef IOT_PORTAL_AP_PREFIX
  #define IOT_PORTAL_AP_PREFIX "IoT-Config-"
#endif

#ifndef IOT_PORTAL_AP_PASSWORD
  #define IOT_PORTAL_AP_PASSWORD "iot123456"
#endif

#ifndef IOT_PORTAL_TITLE
  #define IOT_PORTAL_TITLE "IoT WiFi 配网"
#endif

#ifndef IOT_PORTAL_INFO_HTML
  #define IOT_PORTAL_INFO_HTML "<p>填写 WiFi、MQTT 与设备信息，保存后设备会连接 MQTT 并订阅 cmd。</p>"
#endif

#ifndef IOT_PORTAL_HEAD_HTML
  #define IOT_PORTAL_HEAD_HTML "<style>body{font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',sans-serif;background:#f3f4f6;}button{border-radius:12px!important;background:#2563eb!important;}input{border-radius:10px!important;}</style>"
#endif

struct IotMiniCommand {
  String rawTopic;
  String rawPayload;
  String msgId;
  String action;
  String widgetId;
  unsigned long receivedMs = 0;
};

using IotMiniCallback = std::function<void(const IotMiniCommand& cmd, JsonObjectConst params)>;
using IotMiniSimpleCallback = std::function<void()>;

enum class IotMiniHandlerKind : uint8_t {
  WidgetId,
  Action
};

class IotMiniBlinker {
public:
  IotMiniBlinker();

  void config(const char* ssid,
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
              bool mqttUseTls = false);

  void begin();
  void loop();

  void setDebug(bool enabled);
  void setNetworkIndicator(uint8_t pin, uint8_t activeLevel, bool enabled = true);
  void updateNetworkIndicator();

  // WiFiManager 配网入口：可以在按钮长按、MQTT 命令或调试时调用。
  void startConfigPortal();
  void resetSettings(bool restartDevice = true);
  void setPortalButton(uint8_t pin, uint8_t activeLevel, unsigned long holdMs, bool enabled = true);

  bool connected();
  bool wifiConnected() const;
  String topicPrefix() const;
  String topic(const char* subTopic) const;

  const char* deviceId() const { return _deviceId; }
  const char* mqttHost() const { return _mqttHost; }
  uint16_t mqttPort() const { return _mqttPort; }
  bool mqttUseTls() const { return _mqttUseTls; }

  // Blinker 风格入口：按 widgetId 或 action 绑定回调。
  bool attach(const char* widgetId, IotMiniCallback cb);
  bool attachAction(const char* action, IotMiniCallback cb);
  void onAny(IotMiniCallback cb);
  void onConnected(IotMiniSimpleCallback cb);

  // MQTT 输出。
  bool publish(const char* subTopic, const String& payload, bool retained = false);
  bool publishJson(const char* subTopic, JsonDocument& doc, bool retained = false);
  bool publishState(JsonDocument& doc, bool retained = true);
  bool publishMeta(bool retained = true);
  bool publishStatus(bool online, const char* reason, bool retained = true);
  bool publishAck(const IotMiniCommand& cmd, bool ok, const char* message = "ok");
  bool publishEvent(const char* eventType, JsonDocument& doc, bool retained = false);

  // 常用控件输出封装。
  bool print(const char* widgetId, const char* text);
  bool print(const char* widgetId, const String& text);
  bool number(const char* widgetId, float value, const char* unit = nullptr);
  bool text(const char* widgetId, const char* line1, const char* line2 = nullptr);
  bool notify(const char* title, const char* message);
  bool push(const char* title, const char* message);
  bool sms(const char* phoneOrTag, const char* message);
  bool wechat(const char* openIdOrTag, const char* message);
  bool summary(const char* title, const char* message);
  bool debug(const char* message);
  bool debug(const String& message);

  // 参数解析工具。
  static bool paramBool(JsonObjectConst params, const char* key, bool fallback = false);
  static int paramInt(JsonObjectConst params, const char* key, int fallback = 0);
  static float paramFloat(JsonObjectConst params, const char* key, float fallback = 0.0f);
  static String paramString(JsonObjectConst params, const char* key, const char* fallback = "");
  static bool paramLooksOn(JsonVariantConst value, bool fallback = false);
  static bool parseColorHex(const String& color, uint8_t& r, uint8_t& g, uint8_t& b);

private:
  struct HandlerSlot {
    bool used = false;
    IotMiniHandlerKind kind = IotMiniHandlerKind::WidgetId;
    String key;
    IotMiniCallback cb = nullptr;
  };

  WiFiClient _plainClient;
#if defined(ESP8266)
  BearSSL::WiFiClientSecure _secureClient;
#else
  WiFiClientSecure _secureClient;
#endif
  PubSubClient _mqtt;

  char _ssid[64] = {0};
  char _pass[64] = {0};
  char _mqttHost[96] = {0};
  uint16_t _mqttPort = 1883;
  bool _mqttUseTls = false;
  char _mqttUser[48] = {0};
  char _mqttPassword[64] = {0};
  char _accountId[32] = {0};
  char _deviceId[32] = {0};
  char _deviceName[48] = {0};
  char _deviceType[32] = {0};
  char _topicBase[64] = {0};
  char _firmwareName[64] = {0};
  char _firmwareVersion[24] = {0};

  HandlerSlot _handlers[IOT_MAX_HANDLERS];
  IotMiniCallback _anyCallback = nullptr;
  IotMiniSimpleCallback _connectedCallback = nullptr;

  bool _debugEnabled = true;
  bool _networkIndicatorEnabled = false;
  uint8_t _networkIndicatorPin = 255;
  uint8_t _networkIndicatorActiveLevel = HIGH;

  bool _portalButtonEnabled = false;
  bool _portalButtonLastPressed = false;
  bool _portalButtonOpened = false;
  uint8_t _portalButtonPin = 255;
  uint8_t _portalButtonActiveLevel = LOW;
  unsigned long _portalButtonHoldMs = 5000UL;
  unsigned long _portalButtonPressedAt = 0;
  unsigned long _portalButtonLastChangeAt = 0;

  unsigned long _lastWifiAttempt = 0;
  unsigned long _lastMqttAttempt = 0;
  unsigned long _lastHeartbeat = 0;
  String _lastMsgId;

  static IotMiniBlinker* _active;
  static void mqttCallbackStatic(char* topic, uint8_t* payload, unsigned int length);

  bool addHandler(IotMiniHandlerKind kind, const char* key, IotMiniCallback cb);
  void ensureWiFi();
  void ensureMqtt();
  void setupMqttClient();
  void handleMqttMessage(char* topic, uint8_t* payload, unsigned int length);
  void routeCommand(const IotMiniCommand& cmd, JsonObjectConst params);
  void handlePortalButton();
  bool readPortalButton() const;

  bool mountFs();
  bool loadStoredSettings();
  bool saveStoredSettings();
  bool connectWiFiWithManager(bool forcePortal);
  void copyText(char* dst, size_t dstSize, const char* src);

  String makeClientId() const;
  String ipText() const;
  String chipText() const;
  bool hasMqttUser() const;
};
