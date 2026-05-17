// 示例片段：Button + Switch + Debug
// 这个文件是阅读示例，不参与 PlatformIO 默认编译。

#include <IotMiniBlinker.h>

extern IotMiniBlinker Iot;

void handleButtonDebug(const IotMiniCommand& cmd, JsonObjectConst params) {
  String value = IotMiniBlinker::paramString(params, "value", "tap");
  Iot.print("print_debug", String("Button: ") + value);
  Iot.publishAck(cmd, true, "button ok");
}

void handleSwitchRelay(const IotMiniCommand& cmd, JsonObjectConst params) {
  bool relay = IotMiniBlinker::paramBool(params, "relay", false);
  // digitalWrite(RELAY_PIN, relay ? RELAY_ACTIVE_LEVEL : !RELAY_ACTIVE_LEVEL);
  Iot.publishAck(cmd, true, relay ? "relay on" : "relay off");
}

void registerExample01() {
  Iot.attach("btn_debug", handleButtonDebug);
  Iot.attach("w_lanrenkaiguan_relay", handleSwitchRelay);
  Iot.attach("switch_relay", handleSwitchRelay);
}
