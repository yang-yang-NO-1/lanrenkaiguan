# IotMiniBlinker All Widgets WiFiManager v1.4

本版在 v1.3 在线状态修复基础上，增加 SG90 舵机脉宽工程内校准：

```cpp
#define IOT_LAZY_SERVO_MIN_PULSE_US   500
#define IOT_LAZY_SERVO_MAX_PULSE_US   2400
```

程序通过：

```cpp
lazyServo.attach(IOT_LAZY_SERVO_PIN, IOT_LAZY_SERVO_MIN_PULSE_US, IOT_LAZY_SERVO_MAX_PULSE_US);
```

实现与修改官方 `Servo.h` 中 `DEFAULT_MIN_PULSE_WIDTH=500`、`DEFAULT_MAX_PULSE_WIDTH=2400` 类似的角度效果，但不需要改 PlatformIO 下载的官方 Servo 库。

---

# IotMiniBlinker All Widgets + WiFiManager 完整工程 v1.3

这是给“懒人开关”设备使用的完整 PlatformIO 工程：

- 保留 WiFiManager 配网。
- 保留 EMQX / MQTT 通信。
- 保留设备账户 `device`。
- 保留你的真实舵机懒人开关动作。
- 补齐 Blinker 风格控件示例。
- 不上传温湿度。
- 断网指示灯逻辑：断网亮，WiFi + MQTT 都连上后灭。

本工程不是强依赖点灯官方云端，而是按 Blinker 控件风格做了一层 `IotMiniBlinker` MQTT 封装，适配你现在的小程序 topic：

```text
u/home/devices/{deviceId}/cmd
u/home/devices/{deviceId}/state
u/home/devices/{deviceId}/status
u/home/devices/{deviceId}/ack
u/home/devices/{deviceId}/meta
u/home/devices/{deviceId}/event
```

默认设备：

```text
accountId: device
deviceId : lanrenkaiguan
topic    : u/home/devices/lanrenkaiguan
```

## 工程结构

```text
IotMiniBlinker_AllWidgets_WiFiManager_v1_3_online_fix/
├─ platformio.ini
├─ include/
│  ├─ iot_device_config.h
│  └─ iot_device_config.example.h
├─ src/
│  └─ main.cpp
├─ lib/
│  ├─ IotMiniBlinker/
│  │  ├─ library.json
│  │  └─ src/
│  │     ├─ IotMiniBlinker.h
│  │     └─ IotMiniBlinker.cpp
│  └─ WiFiManager/              # 已从你的原工程恢复到本地 lib
├─ docs/
│  ├─ CONTROL_WIDGETS.md
│  ├─ MQTT_PROTOCOL.md
│  └─ APP_WIDGET_JSON_EXAMPLES.md
└─ examples/
   ├─ 01_button_switch_debug.cpp
   ├─ 02_slider_rgb_ws2812.cpp
   └─ 03_notify_time_auto.cpp
```

## WiFiManager 怎么用

首次烧录后，如果设备没有保存过 WiFi，会自动进入配网模式。

默认 AP：

```text
SSID:     IoT-Config-lanrenkaiguan
Password: iot123456
```

连接这个热点后进入配网页，填写：

```text
WiFi SSID
WiFi Password
MQTT Host
MQTT Port
MQTT TLS: 1/0
MQTT Username
MQTT Password
Account ID
Topic Base
Device ID
Device Name
Device Type
```

默认 MQTT 参数已经按你的工程放好：

```cpp
#define IOT_MQTT_HOST      "a1918f6c.ala.cn-hangzhou.emqxsl.cn"
#define IOT_MQTT_PORT      8883
#define IOT_MQTT_USE_TLS   1
#define IOT_MQTT_USER      "device"
#define IOT_MQTT_PASSWORD  "device"
```

如果你改用本地 EMQX 的 `1883` 明文端口，配网页里把 `MQTT TLS: 1/0` 改成 `0`。

## 长按进配网

默认长按 `D7/GPIO13` 超过 5 秒进入 WiFiManager 配网页：

```cpp
#define IOT_PORTAL_BUTTON_PIN          D7
#define IOT_PORTAL_BUTTON_ACTIVE_LEVEL LOW
#define IOT_FORCE_PORTAL_HOLD_MS       5000UL
```

也可以通过 MQTT 打开配网页：

```json
{
  "msgId": "cmd_open_portal_001",
  "action": "openPortal",
  "widgetId": "btn_open_portal",
  "params": {}
}
```

清空 WiFiManager 和 MQTT 保存参数并重启：

```json
{
  "msgId": "cmd_reset_settings_001",
  "action": "resetSettings",
  "widgetId": "btn_reset_settings",
  "params": {}
}
```

## 编译上传

```bash
pio run -e nodemcuv2
pio run -e nodemcuv2 -t upload
pio device monitor -b 115200
```

ESP32 可以用：

```bash
pio run -e esp32dev
pio run -e esp32dev -t upload
```

## 懒人开关命令测试

你之前的小程序发的是这种格式：

```text
[mqtt publish] u/home/devices/lanrenkaiguan/cmd {"msgId":"cmd_xxx","action":"set","params":{"relay":0},"widgetId":"w_lanrenkaiguan_relay"}
```

这个工程已经专门兼容。

打开：

```bash
mqttx pub -t "u/home/devices/lanrenkaiguan/cmd" -m '{"msgId":"cmd_test_on","action":"set","params":{"relay":1},"widgetId":"w_lanrenkaiguan_relay"}'
```

关闭：

```bash
mqttx pub -t "u/home/devices/lanrenkaiguan/cmd" -m '{"msgId":"cmd_test_off","action":"set","params":{"relay":0},"widgetId":"w_lanrenkaiguan_relay"}'
```

设备端收到后会执行：

```cpp
moveLazySwitch(true / false)
```

也就是实际驱动舵机动作，而不是只改一个普通继电器 GPIO。

## 断网指示灯逻辑

工程已经实现：

```text
WiFi 或 MQTT 任意一个没连上：断网指示灯亮
WiFi + MQTT 都连上：断网指示灯灭
```

默认使用 NodeMCU 内置 LED：

```cpp
#define NETWORK_LED_PIN           LED_BUILTIN
#define NETWORK_LED_ACTIVE_LEVEL  LOW
```

NodeMCU 内置 LED 通常是低电平点亮，所以默认 `LOW`。如果你用外接 LED 且高电平点亮，把它改成：

```cpp
#define NETWORK_LED_ACTIVE_LEVEL  HIGH
```

## 不上传温湿度

本工程没有 DHT 代码，也不会上传 `temperature` / `humidity`，也不会向 `telemetry` topic 上传温湿度。

设备只会发布：

```text
state   当前状态
status  在线/离线/心跳
ack     命令执行结果
meta    设备元信息
event   控件事件、调试信息、通知类事件
```

## 已实现控件

| 控件 | widgetId / action | 设备端处理 |
|---|---|---|
| Button | `btn_debug` / `btn_print` | 按钮事件、调试输出 |
| Switch | `w_lanrenkaiguan_relay` / `switch_relay` | 懒人开关舵机动作 |
| Slider | `sli_brightness` | 亮度值 0~255，示例 PWM 输出 |
| RGB | `rgb_color` | `#RRGGBB` 颜色解析 |
| Number | `num_value` | 数字输入/显示 |
| Text | `tex_status` | 文本显示/更新 |
| Joystick | `joy_xy` | x/y 摇杆值 |
| Image | `img_main` | 图片 URL/资源信息事件 |
| Heartbeat | action: `get` / `sync` / `state` | 状态同步和心跳 |
| Notify | `btn_notify` | 发送 notify 事件 |
| PUSH | `btn_push` | 发送 push 事件 |
| SMS | `btn_sms` | 发送 sms 事件 |
| WECHAT | `btn_wechat` | 发送 wechat 事件 |
| Summary | `btn_summary` | 发送 summary 事件 |
| Tab | `tab_main` | tab/index 切换 |
| Time | `time_main` | 定时/时间参数 |
| AUTO | `auto_rule` | 自动化规则参数 |
| Print/Debug | `btn_print` / `print_debug` | 串口 + MQTT event 调试 |
| WS2812 | `ws2812_strip` | 灯带控件示例入口，默认不引入灯带库 |
| 配网 | `btn_open_portal` / action `openPortal` | 打开 WiFiManager 配网页 |
| 重置 | `btn_reset_settings` / action `resetSettings` | 清空 WiFiManager 和 MQTT 保存参数 |
| 电源 | `btn_power` / action `power` | 执行原懒人开关关机序列 |

详细 payload 看：

```text
docs/CONTROL_WIDGETS.md
docs/MQTT_PROTOCOL.md
docs/APP_WIDGET_JSON_EXAMPLES.md
```

## WS2812 说明

为了保证你现在没有灯带也能直接编译，`ws2812_strip` 只做控件命令接收和状态上报，不强制引入 `FastLED` 或 `Adafruit_NeoPixel`。

后续真接灯带时，把 `src/main.cpp` 里的：

```cpp
void handleWS2812(...)
```

替换成真实灯带驱动即可。
