# MQTT 协议说明

## Topic 规则

设备 ID：

```text
lanrenkaiguan
```

设备 topic 前缀：

```text
u/home/devices/lanrenkaiguan
```

完整 topic：

| Topic | 方向 | retain | 说明 |
|---|---:|---:|---|
| `u/home/devices/lanrenkaiguan/cmd` | 小程序/服务器 → 设备 | false | 下发控件命令 |
| `u/home/devices/lanrenkaiguan/state` | 设备 → 小程序/服务器 | true | 设备当前状态 |
| `u/home/devices/lanrenkaiguan/status` | 设备 → 小程序/服务器 | true | 在线、离线、心跳 |
| `u/home/devices/lanrenkaiguan/ack` | 设备 → 小程序/服务器 | false | 命令执行结果 |
| `u/home/devices/lanrenkaiguan/meta` | 设备 → 小程序/服务器 | true | 设备元信息、控件列表 |
| `u/home/devices/lanrenkaiguan/event` | 设备 → 小程序/服务器 | false | debug、notify、push、summary 等事件 |

## 通用命令格式

```json
{
  "msgId": "cmd_unique_id",
  "action": "set",
  "widgetId": "w_lanrenkaiguan_relay",
  "params": {
    "relay": 1
  },
  "ts": 1779010036645,
  "timeoutMs": 8000,
  "fastResolveMs": 700
}
```

字段说明：

| 字段 | 必填 | 说明 |
|---|---:|---|
| `msgId` | 推荐 | 命令唯一 ID，用于 ack 对应；设备端会忽略连续重复 msgId |
| `action` | 推荐 | `set` / `toggle` / `get` / `sync` / `state` 等 |
| `widgetId` | 推荐 | 控件 ID，用于路由到对应处理函数 |
| `params` | 推荐 | 控件参数 |
| `ts` | 可选 | 小程序/服务器时间戳 |
| `timeoutMs` | 可选 | 小程序侧超时时间 |
| `fastResolveMs` | 可选 | 小程序侧快速响应时间 |

## Ack 格式

设备收到并处理命令后发布：

```text
u/home/devices/lanrenkaiguan/ack
```

示例：

```json
{
  "deviceId": "lanrenkaiguan",
  "msgId": "cmd_test_on",
  "widgetId": "w_lanrenkaiguan_relay",
  "action": "set",
  "ok": true,
  "message": "relay on",
  "ts": 123456
}
```

## State 格式

设备状态发布到：

```text
u/home/devices/lanrenkaiguan/state
```

示例：

```json
{
  "deviceId": "lanrenkaiguan",
  "relay": 1,
  "brightness": 128,
  "color": "#00AAFF",
  "mode": "manual",
  "joystickX": 0,
  "joystickY": 0,
  "numberValue": 12.3,
  "textValue": "ready",
  "activeTab": "home",
  "imageUrl": "",
  "timerText": "",
  "autoRule": "",
  "ws2812Effect": "off",
  "rssi": -52,
  "uptimeMs": 123456
}
```

注意：这个 state 中没有 `temperature` / `humidity`。

## Status / LWT

设备上线发布：

```json
{
  "deviceId": "lanrenkaiguan",
  "online": true,
  "status": "online",
  "reason": "mqtt_connected",
  "ip": "192.168.0.123",
  "rssi": -52,
  "uptimeMs": 123456,
  "freeHeap": 42000
}
```

设备异常断线时，MQTT LWT 会把 retained status 改成：

```json
{
  "deviceId": "lanrenkaiguan",
  "online": false,
  "status": "offline",
  "reason": "lwt"
}
```

## Meta 格式

设备连接成功后发布 retained meta：

```json
{
  "version": 1,
  "deviceId": "lanrenkaiguan",
  "name": "懒人开关",
  "type": "switch",
  "accountId": "device",
  "topicPrefix": "u/home/devices/lanrenkaiguan",
  "fw": "iot-mini-blinker-all-widgets",
  "fwVersion": "1.0.0",
  "protocol": "iot-mini-blinker-mqtt",
  "cmdTopic": "u/home/devices/lanrenkaiguan/cmd",
  "stateTopic": "u/home/devices/lanrenkaiguan/state",
  "statusTopic": "u/home/devices/lanrenkaiguan/status",
  "ackTopic": "u/home/devices/lanrenkaiguan/ack",
  "eventTopic": "u/home/devices/lanrenkaiguan/event",
  "widgets": [
    "btn_debug",
    "btn_print",
    "w_lanrenkaiguan_relay",
    "switch_relay",
    "sli_brightness",
    "rgb_color",
    "num_value",
    "tex_status",
    "joy_xy",
    "img_main",
    "tab_main",
    "time_main",
    "auto_rule",
    "ws2812_strip",
    "btn_notify",
    "btn_push",
    "btn_sms",
    "btn_wechat",
    "btn_summary"
  ]
}
```

## Event 格式

`event` 主要用于调试、通知、文本输出等非状态类消息：

```json
{
  "deviceId": "lanrenkaiguan",
  "eventType": "debug",
  "message": "device online",
  "ts": 123456
}
```

也可能是：

```json
{
  "deviceId": "lanrenkaiguan",
  "eventType": "notify",
  "title": "设备通知",
  "message": "Notify 示例触发",
  "ts": 123456
}
```

## WiFiManager 相关

本工程 v1.2 已恢复 WiFiManager。MQTT 参数会保存到 LittleFS：

```text
/iot_mini_blinker.json
```

WiFi 凭据由 WiFiManager/ESP8266 Arduino core 自身保存。

默认配网 AP：

```text
SSID: IoT-Config-lanrenkaiguan
Password: iot123456
```

长按 `IOT_PORTAL_BUTTON_PIN` 超过 `IOT_FORCE_PORTAL_HOLD_MS` 会重新打开配网页。

也可以通过 MQTT 命令打开配网页：

```json
{
  "msgId": "cmd_open_portal_001",
  "action": "openPortal",
  "widgetId": "btn_open_portal",
  "params": {}
}
```
