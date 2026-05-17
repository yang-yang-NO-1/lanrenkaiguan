#pragma once

/*
 * iot_device_config.h
 *
 * v1.4：WiFiManager 已恢复；SG90 舵机脉宽改为工程内配置，不修改官方 Servo 库。
 * 首次启动或长按配网按钮会出现 AP：IoT-Config-lanrenkaiguan
 * 默认密码：iot123456
 * 配网页里可以修改 WiFi、MQTT、TLS、设备 ID、Topic Base 等信息。
 */

// ========== WiFiManager ==========
#define IOT_ENABLE_WIFI_MANAGER       1
#define IOT_PORTAL_AP_PREFIX          "IoT-Config-"
#define IOT_PORTAL_AP_PASSWORD        "iot123456"
#define IOT_PORTAL_TITLE              "懒人开关 IoT 配网"
#define IOT_PORTAL_TIMEOUT_SEC        180
#define IOT_PORTAL_CONNECT_TIMEOUT_SEC 20
#define IOT_PORTAL_INFO_HTML          "<p>填写 WiFi、EMQX 与设备信息。保存后设备会连接 EMQX，并订阅 u/home/devices/{deviceId}/cmd。</p>"
#define IOT_PORTAL_HEAD_HTML          "<style>body{font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',sans-serif;background:#f3f4f6;} .wrap{max-width:520px;margin:0 auto;} h1{color:#111827;} button{border-radius:12px!important;background:#2563eb!important;} input{border-radius:10px!important;}</style>"

// 静态 WiFi 只在关闭 WiFiManager 时使用；当前默认不用。
#define IOT_WIFI_SSID                 ""
#define IOT_WIFI_PASSWORD             ""

// ========== MQTT / EMQX 默认参数 ==========
#define IOT_MQTT_HOST                 "a1918f6c.ala.cn-hangzhou.emqxsl.cn"
#define IOT_MQTT_PORT                 8883
#define IOT_MQTT_USE_TLS              1
#define IOT_MQTT_USER                 "device"
#define IOT_MQTT_PASSWORD             "device"

// ========== 设备身份 ==========
#define IOT_ACCOUNT_ID                "device"
#define IOT_DEVICE_ID                 "lanrenkaiguan"
#define IOT_DEVICE_NAME               "懒人开关"
#define IOT_DEVICE_TYPE               "servo_switch"
#define IOT_FIRMWARE_NAME             "lanrenkaiguan-iot-mini-blinker"
#define IOT_FIRMWARE_VER              "1.4.0"

// topicPrefix = u/home/devices/{deviceId}
// 命令 topic = u/home/devices/lanrenkaiguan/cmd
#define IOT_TOPIC_BASE                "u/home/devices"

// ========== MQTT 行为 ==========
#define IOT_HEARTBEAT_MS              30000UL
#define IOT_WIFI_RETRY_MS             5000UL
#define IOT_MQTT_RETRY_MS             3000UL

// ========== 断网指示灯 ==========
#ifndef LED_BUILTIN
  #define LED_BUILTIN 2
#endif

#define NETWORK_LED_PIN               LED_BUILTIN
// NodeMCU 内置 LED 多数 LOW 点亮：断网亮，WiFi+MQTT 都在线后灭。
#define NETWORK_LED_ACTIVE_LEVEL      LOW

// ========== 长按配网按钮 ==========
#ifndef D7
  #define D7 13
#endif
#define IOT_PORTAL_BUTTON_PIN         D7
#define IOT_PORTAL_BUTTON_ACTIVE_LEVEL LOW
#define IOT_FORCE_PORTAL_HOLD_MS      5000UL

// ========== 懒人开关硬件参数 ==========
#define IOT_LAZY_SERVO_PIN            4   // D2/GPIO4 舵机信号脚
#define IOT_LAZY_SERVO_POWER_PIN      14  // D5/GPIO14 舵机供电/MOS 控制
#define IOT_LAZY_POWER_HOLD_PIN       12  // D6/GPIO12 IP5306 维持常开控制
#define IOT_LAZY_SERVO_ZERO_ANGLE     95
#define IOT_LAZY_SERVO_DELTA_ANGLE    20

// SG90 舵机校准脉宽：等价于你原来修改 Servo 库 DEFAULT_MIN=500、DEFAULT_MAX=2400 的效果。
// 这里通过 servo.attach(pin, minUs, maxUs) 生效，不再修改官方 Servo 库。
#define IOT_LAZY_SERVO_MIN_PULSE_US   500
#define IOT_LAZY_SERVO_MAX_PULSE_US   2400
#define IOT_LAZY_SERVO_NEUTRAL_US     1500

#define IOT_LAZY_SERVO_SETTLE_MS      300
#define IOT_LAZY_SERVO_RETURN_MS      100

// ========== 可选演示 PWM 输出 ==========
#ifndef D1
  #define D1 5
#endif
#define IOT_DEMO_PWM_PIN              D1
