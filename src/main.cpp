#include <Arduino.h>
#include <ArduinoJson.h>
#include <Servo.h>

#include "iot_device_config.h"
#include <IotMiniBlinker.h>

/*
 * 完整演示工程 v1.4：
 * - WiFiManager 配网已恢复：首次启动/长按按钮进入 AP 配网。
 * - MQTT topic: u/home/devices/{deviceId}/cmd/state/status/ack/meta/event
 * - 设备账户默认：device
 * - 控件：Button、Switch、Slider、RGB、Number、Text、Joystick、Image、Heartbeat、Notify、PUSH、SMS、WECHAT、Summary、Tab、Time、AUTO、Print/Debug、WS2812
 * - 懒人开关真实动作：w_lanrenkaiguan_relay 会驱动舵机完成开/关动作。
 * - SG90 舵机脉宽：通过 lazyServo.attach(pin, 500, 2400) 校准，不修改官方 Servo 库。
 * - 断网指示灯：断网亮，WiFi+MQTT 都连上后灭。
 * - 不上传温湿度：没有 DHT，也不会 publish telemetry 温湿度。
 */

IotMiniBlinker Iot;
Servo lazyServo;
static bool lazyServoIsAttached = false;

struct DeviceState {
  bool relay = false;
  int brightness = 0;          // 0~255，仅用于控件演示/PWM 演示
  String color = "#000000";
  String mode = "manual";
  int joystickX = 0;
  int joystickY = 0;
  float numberValue = 0;
  String textValue = "ready";
  String activeTab = "home";
  String imageUrl = "";
  String timerText = "";
  String autoRule = "";
  String ws2812Effect = "off";
};

DeviceState state;

static bool hasParam(JsonObjectConst params, const char* key) {
  return !params.isNull() && key && params.containsKey(key);
}

static void servoEnable(bool enabled) {
  digitalWrite(IOT_LAZY_SERVO_POWER_PIN, enabled ? HIGH : LOW);
}

static void attachLazyServoCalibrated() {
  // 不修改官方 Servo 库，直接在工程里指定 SG90 的角度映射脉宽范围。
  // 等价于把 Servo 库 DEFAULT_MIN_PULSE_WIDTH 改成 500、DEFAULT_MAX_PULSE_WIDTH 改成 2400。
  if (!lazyServoIsAttached) {
    lazyServo.attach(
      IOT_LAZY_SERVO_PIN,
      IOT_LAZY_SERVO_MIN_PULSE_US,
      IOT_LAZY_SERVO_MAX_PULSE_US
    );
    lazyServoIsAttached = true;
  }
}

static void detachLazyServoSafe() {
  if (lazyServoIsAttached) {
    lazyServo.detach();
    lazyServoIsAttached = false;
  }
  pinMode(IOT_LAZY_SERVO_PIN, OUTPUT);
  digitalWrite(IOT_LAZY_SERVO_PIN, LOW);
}

static void lazyServoWriteAngle(int angle, uint32_t holdMs) {
  attachLazyServoCalibrated();
  lazyServo.write(constrain(angle, 0, 180));
  delay(holdMs);
}

static bool moveLazySwitch(bool on) {
  Serial.print(F("[lazy] switch -> "));
  Serial.println(on ? F("on") : F("off"));

  servoEnable(true);
  delay(30);
  attachLazyServoCalibrated();

  if (on) {
    lazyServoWriteAngle(IOT_LAZY_SERVO_ZERO_ANGLE - IOT_LAZY_SERVO_DELTA_ANGLE - 5, IOT_LAZY_SERVO_SETTLE_MS);
    lazyServoWriteAngle(IOT_LAZY_SERVO_ZERO_ANGLE - 1, IOT_LAZY_SERVO_RETURN_MS);
  } else {
    lazyServoWriteAngle(IOT_LAZY_SERVO_ZERO_ANGLE + IOT_LAZY_SERVO_DELTA_ANGLE + 5, IOT_LAZY_SERVO_SETTLE_MS);
    lazyServoWriteAngle(IOT_LAZY_SERVO_ZERO_ANGLE + 1, IOT_LAZY_SERVO_RETURN_MS);
  }

  servoEnable(false);
  delay(10);
  detachLazyServoSafe();
  return true;
}

static bool powerOffLazySwitch() {
  Serial.println(F("[lazy] power off sequence"));
  digitalWrite(IOT_LAZY_POWER_HOLD_PIN, LOW);
  delay(100);
  digitalWrite(IOT_LAZY_POWER_HOLD_PIN, HIGH);
  delay(100);
  digitalWrite(IOT_LAZY_POWER_HOLD_PIN, LOW);
  delay(100);
  digitalWrite(IOT_LAZY_POWER_HOLD_PIN, HIGH);
  return true;
}

void publishFullState() {
  DynamicJsonDocument doc(1536);
  doc["deviceId"] = Iot.deviceId();
  doc["relay"] = state.relay ? 1 : 0;
  doc["relayText"] = state.relay ? "on" : "off";
  doc["brightness"] = state.brightness;
  doc["color"] = state.color;
  doc["mode"] = state.mode;
  doc["joystickX"] = state.joystickX;
  doc["joystickY"] = state.joystickY;
  doc["numberValue"] = state.numberValue;
  doc["textValue"] = state.textValue;
  doc["activeTab"] = state.activeTab;
  doc["imageUrl"] = state.imageUrl;
  doc["timerText"] = state.timerText;
  doc["autoRule"] = state.autoRule;
  doc["ws2812Effect"] = state.ws2812Effect;
  doc["mqttHost"] = Iot.mqttHost();
  doc["mqttPort"] = Iot.mqttPort();
  doc["mqttUseTls"] = Iot.mqttUseTls();
  doc["rssi"] = Iot.wifiConnected() ? WiFi.RSSI() : 0;
  doc["online"] = Iot.connected();
  doc["uptimeMs"] = millis();
  doc["uptime"] = (uint32_t)(millis() / 1000UL);

  // 注意：这里没有 temperature / humidity，也没有 telemetry 温湿度上报。
  Iot.publishState(doc, true);
}

void setLogicalRelay(bool on, bool doPhysicalAction) {
  if (doPhysicalAction) {
    moveLazySwitch(on);
  }
  state.relay = on;
  publishFullState();
}

void handleGetState(const IotMiniCommand& cmd, JsonObjectConst params) {
  (void)params;
  publishFullState();
  Iot.publishAck(cmd, true, "state published");
}

void handleButtonDebug(const IotMiniCommand& cmd, JsonObjectConst params) {
  String value = IotMiniBlinker::paramString(params, "value", "tap");
  if (hasParam(params, "button")) value = IotMiniBlinker::paramString(params, "button", value.c_str());

  String msg = String("Button event: widget=") + cmd.widgetId + ", value=" + value;
  Iot.print("print_debug", msg);
  Iot.debug(msg);
  Iot.publishAck(cmd, true, "button handled");
}

void handleSwitchRelay(const IotMiniCommand& cmd, JsonObjectConst params) {
  bool next = state.relay;

  // 兼容你现在小程序发来的格式：params: {"relay":0/1}
  if (hasParam(params, "relay")) {
    next = IotMiniBlinker::paramBool(params, "relay", state.relay);
  } else if (hasParam(params, "state")) {
    next = IotMiniBlinker::paramBool(params, "state", state.relay);
  } else if (hasParam(params, "switch")) {
    next = IotMiniBlinker::paramBool(params, "switch", state.relay);
  } else if (hasParam(params, "value")) {
    next = IotMiniBlinker::paramBool(params, "value", state.relay);
  } else if (cmd.action == "toggle") {
    next = !state.relay;
  }

  setLogicalRelay(next, true);
  Iot.publishAck(cmd, true, next ? "lazy switch on" : "lazy switch off");
}

void handlePower(const IotMiniCommand& cmd, JsonObjectConst params) {
  (void)params;
  bool ok = powerOffLazySwitch();
  Iot.publishAck(cmd, ok, ok ? "power off sequence executed" : "power off failed");
}

void handleResetSettings(const IotMiniCommand& cmd, JsonObjectConst params) {
  (void)params;
  Iot.publishAck(cmd, true, "reset settings and restart");
  delay(100);
  Iot.resetSettings(true);
}

void handleOpenPortal(const IotMiniCommand& cmd, JsonObjectConst params) {
  (void)params;
  Iot.publishAck(cmd, true, "opening config portal");
  delay(100);
  Iot.startConfigPortal();
}

void handleSliderBrightness(const IotMiniCommand& cmd, JsonObjectConst params) {
  int value = state.brightness;
  if (hasParam(params, "brightness")) value = IotMiniBlinker::paramInt(params, "brightness", value);
  else if (hasParam(params, "value")) value = IotMiniBlinker::paramInt(params, "value", value);
  else if (hasParam(params, "slider")) value = IotMiniBlinker::paramInt(params, "slider", value);

  state.brightness = constrain(value, 0, 255);
  analogWrite(IOT_DEMO_PWM_PIN, state.brightness);
  publishFullState();
  Iot.number("num_brightness", state.brightness, "level");
  Iot.publishAck(cmd, true, "brightness updated");
}

void handleRGBColor(const IotMiniCommand& cmd, JsonObjectConst params) {
  String color = state.color;

  if (hasParam(params, "color")) {
    color = IotMiniBlinker::paramString(params, "color", color.c_str());
  } else if (hasParam(params, "r") && hasParam(params, "g") && hasParam(params, "b")) {
    int r = constrain(IotMiniBlinker::paramInt(params, "r", 0), 0, 255);
    int g = constrain(IotMiniBlinker::paramInt(params, "g", 0), 0, 255);
    int b = constrain(IotMiniBlinker::paramInt(params, "b", 0), 0, 255);
    char buf[8];
    snprintf(buf, sizeof(buf), "#%02X%02X%02X", r, g, b);
    color = buf;
  }

  uint8_t r = 0, g = 0, b = 0;
  if (IotMiniBlinker::parseColorHex(color, r, g, b)) {
    state.color = color;
    String msg = String("RGB updated: ") + state.color + " r=" + r + " g=" + g + " b=" + b;
    Iot.print("print_debug", msg);
    publishFullState();
    Iot.publishAck(cmd, true, "rgb updated");
  } else {
    Iot.publishAck(cmd, false, "invalid color, use #RRGGBB");
  }
}

void handleJoystick(const IotMiniCommand& cmd, JsonObjectConst params) {
  state.joystickX = IotMiniBlinker::paramInt(params, "x", state.joystickX);
  state.joystickY = IotMiniBlinker::paramInt(params, "y", state.joystickY);
  publishFullState();
  Iot.publishAck(cmd, true, "joystick updated");
}

void handleNumber(const IotMiniCommand& cmd, JsonObjectConst params) {
  if (hasParam(params, "value")) state.numberValue = IotMiniBlinker::paramFloat(params, "value", state.numberValue);
  else if (hasParam(params, "number")) state.numberValue = IotMiniBlinker::paramFloat(params, "number", state.numberValue);

  Iot.number("num_value", state.numberValue, "value");
  publishFullState();
  Iot.publishAck(cmd, true, "number updated");
}

void handleText(const IotMiniCommand& cmd, JsonObjectConst params) {
  if (hasParam(params, "text")) state.textValue = IotMiniBlinker::paramString(params, "text", state.textValue.c_str());
  else if (hasParam(params, "value")) state.textValue = IotMiniBlinker::paramString(params, "value", state.textValue.c_str());
  else if (hasParam(params, "msg")) state.textValue = IotMiniBlinker::paramString(params, "msg", state.textValue.c_str());

  Iot.text("tex_status", "文本已更新", state.textValue.c_str());
  publishFullState();
  Iot.publishAck(cmd, true, "text updated");
}

void handleImage(const IotMiniCommand& cmd, JsonObjectConst params) {
  if (hasParam(params, "url")) state.imageUrl = IotMiniBlinker::paramString(params, "url", state.imageUrl.c_str());
  else if (hasParam(params, "src")) state.imageUrl = IotMiniBlinker::paramString(params, "src", state.imageUrl.c_str());
  else if (hasParam(params, "image")) state.imageUrl = IotMiniBlinker::paramString(params, "image", state.imageUrl.c_str());

  DynamicJsonDocument doc(768);
  doc["widgetId"] = "img_main";
  doc["url"] = state.imageUrl;
  Iot.publishEvent("image", doc, false);
  publishFullState();
  Iot.publishAck(cmd, true, "image updated");
}

void handleTab(const IotMiniCommand& cmd, JsonObjectConst params) {
  if (hasParam(params, "tab")) state.activeTab = IotMiniBlinker::paramString(params, "tab", state.activeTab.c_str());
  else if (hasParam(params, "value")) state.activeTab = IotMiniBlinker::paramString(params, "value", state.activeTab.c_str());
  else if (hasParam(params, "index")) state.activeTab = String(IotMiniBlinker::paramInt(params, "index", 0));

  publishFullState();
  Iot.publishAck(cmd, true, "tab updated");
}

void handleTime(const IotMiniCommand& cmd, JsonObjectConst params) {
  if (hasParam(params, "time")) state.timerText = IotMiniBlinker::paramString(params, "time", state.timerText.c_str());
  else if (hasParam(params, "cron")) state.timerText = IotMiniBlinker::paramString(params, "cron", state.timerText.c_str());
  else if (hasParam(params, "timerMs")) state.timerText = String(IotMiniBlinker::paramInt(params, "timerMs", 0)) + "ms";

  Iot.text("tex_status", "定时设置", state.timerText.c_str());
  publishFullState();
  Iot.publishAck(cmd, true, "time updated");
}

void handleAuto(const IotMiniCommand& cmd, JsonObjectConst params) {
  if (hasParam(params, "rule")) state.autoRule = IotMiniBlinker::paramString(params, "rule", state.autoRule.c_str());
  else if (hasParam(params, "scene")) state.autoRule = IotMiniBlinker::paramString(params, "scene", state.autoRule.c_str());
  else if (hasParam(params, "value")) state.autoRule = IotMiniBlinker::paramString(params, "value", state.autoRule.c_str());

  Iot.text("tex_status", "自动化规则", state.autoRule.c_str());
  publishFullState();
  Iot.publishAck(cmd, true, "auto rule updated");
}

void handleWS2812(const IotMiniCommand& cmd, JsonObjectConst params) {
  // 这里是 WS2812 控件示例，不强制依赖灯带库，避免没有 WS2812 时编译失败。
  // 真接灯带时，在这里接入 Adafruit_NeoPixel / FastLED 即可。
  if (hasParam(params, "effect")) state.ws2812Effect = IotMiniBlinker::paramString(params, "effect", state.ws2812Effect.c_str());
  else if (hasParam(params, "mode")) state.ws2812Effect = IotMiniBlinker::paramString(params, "mode", state.ws2812Effect.c_str());
  else if (hasParam(params, "color")) state.ws2812Effect = IotMiniBlinker::paramString(params, "color", state.ws2812Effect.c_str());

  Iot.print("print_debug", String("WS2812 effect: ") + state.ws2812Effect);
  publishFullState();
  Iot.publishAck(cmd, true, "ws2812 command accepted");
}

void handleNotify(const IotMiniCommand& cmd, JsonObjectConst params) {
  String title = IotMiniBlinker::paramString(params, "title", "设备通知");
  String msg = IotMiniBlinker::paramString(params, "message", "Notify 示例触发");
  Iot.notify(title.c_str(), msg.c_str());
  Iot.publishAck(cmd, true, "notify event published");
}

void handlePush(const IotMiniCommand& cmd, JsonObjectConst params) {
  String title = IotMiniBlinker::paramString(params, "title", "PUSH 示例");
  String msg = IotMiniBlinker::paramString(params, "message", "Push 示例触发");
  Iot.push(title.c_str(), msg.c_str());
  Iot.publishAck(cmd, true, "push event published");
}

void handleSMS(const IotMiniCommand& cmd, JsonObjectConst params) {
  String target = IotMiniBlinker::paramString(params, "target", "default");
  String msg = IotMiniBlinker::paramString(params, "message", "SMS 示例触发");
  Iot.sms(target.c_str(), msg.c_str());
  Iot.publishAck(cmd, true, "sms event published");
}

void handleWechat(const IotMiniCommand& cmd, JsonObjectConst params) {
  String target = IotMiniBlinker::paramString(params, "target", "default");
  String msg = IotMiniBlinker::paramString(params, "message", "WECHAT 示例触发");
  Iot.wechat(target.c_str(), msg.c_str());
  Iot.publishAck(cmd, true, "wechat event published");
}

void handleSummary(const IotMiniCommand& cmd, JsonObjectConst params) {
  String title = IotMiniBlinker::paramString(params, "title", "设备摘要");
  String msg = IotMiniBlinker::paramString(params, "message", "Summary 示例触发");
  Iot.summary(title.c_str(), msg.c_str());
  Iot.publishAck(cmd, true, "summary event published");
}

void handlePrintDebug(const IotMiniCommand& cmd, JsonObjectConst params) {
  String msg = IotMiniBlinker::paramString(params, "message", "debug print");
  if (hasParam(params, "text")) msg = IotMiniBlinker::paramString(params, "text", msg.c_str());
  Iot.print("print_debug", msg);
  Iot.debug(msg);
  Iot.publishAck(cmd, true, "debug printed");
}

void registerAllWidgetExamples() {
  // Button / Debug
  Iot.attach("btn_debug", handleButtonDebug);
  Iot.attach("btn_print", handlePrintDebug);

  // Switch：包含你现在小程序控件 id。
  Iot.attach("w_lanrenkaiguan_relay", handleSwitchRelay);
  Iot.attach("switch_relay", handleSwitchRelay);

  // Slider / RGB / Number / Text
  Iot.attach("sli_brightness", handleSliderBrightness);
  Iot.attach("rgb_color", handleRGBColor);
  Iot.attach("num_value", handleNumber);
  Iot.attach("tex_status", handleText);

  // Joystick / Image / Tab / Time / AUTO / WS2812
  Iot.attach("joy_xy", handleJoystick);
  Iot.attach("img_main", handleImage);
  Iot.attach("tab_main", handleTab);
  Iot.attach("time_main", handleTime);
  Iot.attach("auto_rule", handleAuto);
  Iot.attach("ws2812_strip", handleWS2812);

  // Notify / PUSH / SMS / WECHAT / Summary
  Iot.attach("btn_notify", handleNotify);
  Iot.attach("btn_push", handlePush);
  Iot.attach("btn_sms", handleSMS);
  Iot.attach("btn_wechat", handleWechat);
  Iot.attach("btn_summary", handleSummary);

  // 系统动作
  Iot.attach("btn_power", handlePower);
  Iot.attach("btn_reset_settings", handleResetSettings);
  Iot.attach("btn_open_portal", handleOpenPortal);

  // Heartbeat / 同步状态动作
  Iot.attachAction("get", handleGetState);
  Iot.attachAction("sync", handleGetState);
  Iot.attachAction("state", handleGetState);
  Iot.attachAction("toggle", handleSwitchRelay);
  Iot.attachAction("power", handlePower);
  Iot.attachAction("shutdown", handlePower);
  Iot.attachAction("resetSettings", handleResetSettings);
  Iot.attachAction("openPortal", handleOpenPortal);
}

void setup() {
  Serial.begin(115200);
  delay(300);

  pinMode(IOT_LAZY_SERVO_POWER_PIN, OUTPUT);
  servoEnable(false);

  pinMode(IOT_LAZY_POWER_HOLD_PIN, OUTPUT);
  digitalWrite(IOT_LAZY_POWER_HOLD_PIN, HIGH);

  pinMode(IOT_DEMO_PWM_PIN, OUTPUT);
#if defined(ESP8266)
  analogWriteRange(255);
#endif
  analogWrite(IOT_DEMO_PWM_PIN, 0);

  // 启动时只初始化舵机信号脚，不常驻 attach，避免断电后信号脚回灌。
  // 真正动作时由 attachLazyServoCalibrated() 使用 500~2400us 脉宽重新 attach。
  detachLazyServoSafe();

  Iot.setDebug(true);
  Iot.setNetworkIndicator(NETWORK_LED_PIN, NETWORK_LED_ACTIVE_LEVEL, true);
  Iot.setPortalButton(IOT_PORTAL_BUTTON_PIN, IOT_PORTAL_BUTTON_ACTIVE_LEVEL, IOT_FORCE_PORTAL_HOLD_MS, true);

  Iot.config(
    IOT_WIFI_SSID,
    IOT_WIFI_PASSWORD,
    IOT_MQTT_HOST,
    IOT_MQTT_PORT,
    IOT_MQTT_USER,
    IOT_MQTT_PASSWORD,
    IOT_ACCOUNT_ID,
    IOT_DEVICE_ID,
    IOT_DEVICE_NAME,
    IOT_DEVICE_TYPE,
    IOT_TOPIC_BASE,
    IOT_FIRMWARE_NAME,
    IOT_FIRMWARE_VER,
    IOT_MQTT_USE_TLS
  );

  registerAllWidgetExamples();

  Iot.onAny([](const IotMiniCommand& cmd, JsonObjectConst params) {
    (void)params;
    Serial.print(F("[cmd] widgetId="));
    Serial.print(cmd.widgetId);
    Serial.print(F(" action="));
    Serial.print(cmd.action);
    Serial.print(F(" payload="));
    Serial.println(cmd.rawPayload);
  });

  Iot.onConnected([]() {
    Iot.debug("device online, publishing state/meta");
    publishFullState();
    Iot.text("tex_status", "设备在线", "WiFiManager 已启用；未上传温湿度");
  });

  Iot.begin();
}

void loop() {
  Iot.loop();
}
