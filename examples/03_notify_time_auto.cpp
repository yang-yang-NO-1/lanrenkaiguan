// 示例片段：Notify / PUSH / SMS / WECHAT / Summary / Time / AUTO
// 这个文件是阅读示例，不参与 PlatformIO 默认编译。

#include <IotMiniBlinker.h>

extern IotMiniBlinker Iot;

void handleNotify(const IotMiniCommand& cmd, JsonObjectConst params) {
  String title = IotMiniBlinker::paramString(params, "title", "设备通知");
  String msg = IotMiniBlinker::paramString(params, "message", "Notify 示例");
  Iot.notify(title.c_str(), msg.c_str());
  Iot.publishAck(cmd, true, "notify ok");
}

void handleTime(const IotMiniCommand& cmd, JsonObjectConst params) {
  String timeText = IotMiniBlinker::paramString(params, "time", "");
  Iot.text("tex_status", "time", timeText.c_str());
  Iot.publishAck(cmd, true, "time ok");
}

void handleAuto(const IotMiniCommand& cmd, JsonObjectConst params) {
  String rule = IotMiniBlinker::paramString(params, "rule", "");
  Iot.text("tex_status", "auto", rule.c_str());
  Iot.publishAck(cmd, true, "auto ok");
}

void registerExample03() {
  Iot.attach("btn_notify", handleNotify);
  Iot.attach("time_main", handleTime);
  Iot.attach("auto_rule", handleAuto);
}
