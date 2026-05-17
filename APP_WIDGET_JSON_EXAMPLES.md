# 小程序/调试端 JSON 示例

下面这些 payload 都 publish 到：

```text
u/home/devices/lanrenkaiguan/cmd
```

## 1. 继电器打开

```json
{
  "msgId": "cmd_relay_on_001",
  "action": "set",
  "widgetId": "w_lanrenkaiguan_relay",
  "params": {
    "relay": 1
  },
  "timeoutMs": 8000,
  "fastResolveMs": 700
}
```

## 2. 继电器关闭

```json
{
  "msgId": "cmd_relay_off_001",
  "action": "set",
  "widgetId": "w_lanrenkaiguan_relay",
  "params": {
    "relay": 0
  },
  "timeoutMs": 8000,
  "fastResolveMs": 700
}
```

## 3. Button

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

## 4. Slider

```json
{
  "msgId": "cmd_slider_001",
  "action": "set",
  "widgetId": "sli_brightness",
  "params": {
    "brightness": 200
  }
}
```

## 5. RGB

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

## 6. Number

```json
{
  "msgId": "cmd_num_001",
  "action": "set",
  "widgetId": "num_value",
  "params": {
    "value": 66.6
  }
}
```

## 7. Text

```json
{
  "msgId": "cmd_text_001",
  "action": "set",
  "widgetId": "tex_status",
  "params": {
    "text": "hello"
  }
}
```

## 8. Joystick

```json
{
  "msgId": "cmd_joy_001",
  "action": "set",
  "widgetId": "joy_xy",
  "params": {
    "x": 50,
    "y": -20
  }
}
```

## 9. Image

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

## 10. 主动同步状态

```json
{
  "msgId": "cmd_sync_001",
  "action": "sync",
  "widgetId": "",
  "params": {}
}
```

## 11. Notify

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

## 12. PUSH

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

## 13. SMS

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

## 14. WECHAT

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

## 15. Summary

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

## 16. Tab

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

## 17. Time

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

## 18. AUTO

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

## 19. Print / Debug

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

## 20. WS2812

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

## MQTTX 命令行示例

打开继电器：

```bash
mqttx pub -h 192.168.0.196 -p 1883 -t "u/home/devices/lanrenkaiguan/cmd" -m '{"msgId":"cmd_relay_on_001","action":"set","widgetId":"w_lanrenkaiguan_relay","params":{"relay":1}}'
```

关闭继电器：

```bash
mqttx pub -h 192.168.0.196 -p 1883 -t "u/home/devices/lanrenkaiguan/cmd" -m '{"msgId":"cmd_relay_off_001","action":"set","widgetId":"w_lanrenkaiguan_relay","params":{"relay":0}}'
```

## 21. 打开 WiFiManager 配网页

```json
{
  "msgId": "cmd_open_portal_001",
  "action": "openPortal",
  "widgetId": "btn_open_portal",
  "params": {}
}
```

## 22. 清空 WiFiManager 与 MQTT 保存参数并重启

```json
{
  "msgId": "cmd_reset_settings_001",
  "action": "resetSettings",
  "widgetId": "btn_reset_settings",
  "params": {}
}
```

## 23. 懒人开关关机序列

```json
{
  "msgId": "cmd_power_001",
  "action": "power",
  "widgetId": "btn_power",
  "params": {}
}
```
