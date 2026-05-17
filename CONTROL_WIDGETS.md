# 控件使用实例

所有命令统一 publish 到：

```text
u/home/devices/lanrenkaiguan/cmd
```

通用外壳：

```json
{
  "msgId": "cmd_xxx",
  "action": "set",
  "widgetId": "控件ID",
  "params": {}
}
```

## 1. Button 按钮控件

widgetId：

```text
btn_debug
btn_print
```

示例：

```json
{
  "msgId": "cmd_btn_001",
  "action": "tap",
  "widgetId": "btn_debug",
  "params": {
    "value": "tap"
  }
}
```

设备动作：串口打印，并向 `event` 发布 debug/print。

## 2. Switch 开关控件

widgetId：

```text
w_lanrenkaiguan_relay
switch_relay
```

打开继电器：

```json
{
  "msgId": "cmd_relay_on",
  "action": "set",
  "widgetId": "w_lanrenkaiguan_relay",
  "params": {
    "relay": 1
  }
}
```

关闭继电器：

```json
{
  "msgId": "cmd_relay_off",
  "action": "set",
  "widgetId": "w_lanrenkaiguan_relay",
  "params": {
    "relay": 0
  }
}
```

也兼容：

```json
{ "state": "on" }
{ "state": "off" }
{ "switch": true }
{ "value": 1 }
```

## 3. Slider 滑块控件

widgetId：

```text
sli_brightness
```

示例：

```json
{
  "msgId": "cmd_slider_001",
  "action": "set",
  "widgetId": "sli_brightness",
  "params": {
    "brightness": 128
  }
}
```

也兼容：

```json
{ "value": 128 }
{ "slider": 128 }
```

范围：`0~255`。

## 4. RGB 颜色控件

widgetId：

```text
rgb_color
```

示例 1：十六进制颜色：

```json
{
  "msgId": "cmd_rgb_001",
  "action": "set",
  "widgetId": "rgb_color",
  "params": {
    "color": "#00AAFF"
  }
}
```

示例 2：RGB 分量：

```json
{
  "msgId": "cmd_rgb_002",
  "action": "set",
  "widgetId": "rgb_color",
  "params": {
    "r": 0,
    "g": 170,
    "b": 255
  }
}
```

## 5. Number 数字控件

widgetId：

```text
num_value
```

示例：

```json
{
  "msgId": "cmd_num_001",
  "action": "set",
  "widgetId": "num_value",
  "params": {
    "value": 12.3
  }
}
```

设备会发布 `eventType=number`，并同步到 `state.numberValue`。

## 6. Text 文本控件

widgetId：

```text
tex_status
```

示例：

```json
{
  "msgId": "cmd_text_001",
  "action": "set",
  "widgetId": "tex_status",
  "params": {
    "text": "hello device"
  }
}
```

也兼容：

```json
{ "value": "hello" }
{ "msg": "hello" }
```

## 7. Joystick 摇杆控件

widgetId：

```text
joy_xy
```

示例：

```json
{
  "msgId": "cmd_joy_001",
  "action": "set",
  "widgetId": "joy_xy",
  "params": {
    "x": 30,
    "y": -60
  }
}
```

## 8. Image 图片控件

widgetId：

```text
img_main
```

示例：

```json
{
  "msgId": "cmd_img_001",
  "action": "set",
  "widgetId": "img_main",
  "params": {
    "url": "https://example.com/demo.png"
  }
}
```

也兼容：

```json
{ "src": "..." }
{ "image": "..." }
```

## 9. Heartbeat 心跳/状态同步

这个不是单独的 UI widget，而是设备端自动上报。

设备每 `IOT_HEARTBEAT_MS` 发布一次：

```text
u/home/devices/lanrenkaiguan/status
```

小程序也可以主动拉取状态：

```json
{
  "msgId": "cmd_sync_001",
  "action": "sync",
  "widgetId": "",
  "params": {}
}
```

也支持：

```json
{ "action": "get" }
{ "action": "state" }
```

## 10. Notify 通知控件

widgetId：

```text
btn_notify
```

示例：

```json
{
  "msgId": "cmd_notify_001",
  "action": "tap",
  "widgetId": "btn_notify",
  "params": {
    "title": "设备通知",
    "message": "Notify 示例触发"
  }
}
```

设备发布：

```text
eventType = notify
```

## 11. PUSH 控件

widgetId：

```text
btn_push
```

示例：

```json
{
  "msgId": "cmd_push_001",
  "action": "tap",
  "widgetId": "btn_push",
  "params": {
    "title": "PUSH 示例",
    "message": "Push 示例触发"
  }
}
```

设备发布：

```text
eventType = push
```

## 12. SMS 控件

widgetId：

```text
btn_sms
```

示例：

```json
{
  "msgId": "cmd_sms_001",
  "action": "tap",
  "widgetId": "btn_sms",
  "params": {
    "target": "default",
    "message": "SMS 示例触发"
  }
}
```

设备发布：

```text
eventType = sms
```

真正发送短信通常应该由服务器端处理，设备只负责发布事件。

## 13. WECHAT 控件

widgetId：

```text
btn_wechat
```

示例：

```json
{
  "msgId": "cmd_wechat_001",
  "action": "tap",
  "widgetId": "btn_wechat",
  "params": {
    "target": "default",
    "message": "WECHAT 示例触发"
  }
}
```

设备发布：

```text
eventType = wechat
```

真正发送微信消息通常由服务器端处理。

## 14. Summary 摘要控件

widgetId：

```text
btn_summary
```

示例：

```json
{
  "msgId": "cmd_summary_001",
  "action": "tap",
  "widgetId": "btn_summary",
  "params": {
    "title": "设备摘要",
    "message": "Summary 示例触发"
  }
}
```

设备发布：

```text
eventType = summary
```

## 15. Tab 标签页控件

widgetId：

```text
tab_main
```

示例：

```json
{
  "msgId": "cmd_tab_001",
  "action": "set",
  "widgetId": "tab_main",
  "params": {
    "tab": "settings"
  }
}
```

也兼容：

```json
{ "value": "settings" }
{ "index": 1 }
```

## 16. Time 时间/定时控件

widgetId：

```text
time_main
```

示例：

```json
{
  "msgId": "cmd_time_001",
  "action": "set",
  "widgetId": "time_main",
  "params": {
    "time": "08:30"
  }
}
```

也兼容：

```json
{ "cron": "30 8 * * *" }
{ "timerMs": 5000 }
```

## 17. AUTO 自动化控件

widgetId：

```text
auto_rule
```

示例：

```json
{
  "msgId": "cmd_auto_001",
  "action": "set",
  "widgetId": "auto_rule",
  "params": {
    "rule": "when relay off then notify"
  }
}
```

也兼容：

```json
{ "scene": "night" }
{ "value": "rule text" }
```

## 18. Print / Debug 控件

widgetId：

```text
btn_print
print_debug
```

示例：

```json
{
  "msgId": "cmd_print_001",
  "action": "tap",
  "widgetId": "btn_print",
  "params": {
    "message": "debug from app"
  }
}
```

设备会：

```text
1. Serial 打印
2. 发布 eventType=debug
3. 发布 eventType=print
```

## 19. WS2812 灯带控件

widgetId：

```text
ws2812_strip
```

示例：

```json
{
  "msgId": "cmd_ws2812_001",
  "action": "set",
  "widgetId": "ws2812_strip",
  "params": {
    "effect": "rainbow",
    "brightness": 120,
    "color": "#FF8800"
  }
}
```

本工程默认只接收命令和上报状态，不直接引入灯带库。需要真实控制灯带时，把 `src/main.cpp` 中的 `handleWS2812()` 替换成具体灯带驱动逻辑即可。

## WiFiManager / 配网相关控件

### 打开配网页

```json
{
  "msgId": "cmd_open_portal_001",
  "action": "openPortal",
  "widgetId": "btn_open_portal",
  "params": {}
}
```

设备会启动 AP：

```text
IoT-Config-lanrenkaiguan
密码：iot123456
```

### 重置 WiFiManager 和 MQTT 参数

```json
{
  "msgId": "cmd_reset_settings_001",
  "action": "resetSettings",
  "widgetId": "btn_reset_settings",
  "params": {}
}
```

### 懒人开关关机序列

```json
{
  "msgId": "cmd_power_001",
  "action": "power",
  "widgetId": "btn_power",
  "params": {}
}
```
