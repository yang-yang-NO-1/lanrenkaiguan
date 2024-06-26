#include <Arduino.h>
#include <ESP8266WiFi.h>
#define BLINKER_WIFI
#include <Blinker.h>
#include <main.h>
#include <Servo.h>

#define PWM1 4         // 舵机驱动引脚
#define zero_angle 95  // 舵机零点角度
#define angle 20       // 舵机动作角度
#define serveSwitch 14 // 舵机上电开关,控制vcc的为13，控制地的为14
#define STAOFF 12      // ip5306维持常开
Ticker timer2;         // 建立Ticker用于实现定时功能
Servo myservo;         // create servo object to control a servo

#define TEXTE1 "T1"
BlinkerText Text1(TEXTE1);
#define BUTTON1 "PWM1"
BlinkerButton Button1(BUTTON1);
#define BUTTON2 "POWER"
BlinkerButton Button2(BUTTON2);

// 打开mos
void serve_enable(bool state)
{
  digitalWrite(serveSwitch, state);
}

// 断开总电源
void PowerOFF()
{
  digitalWrite(STAOFF, 0);
  delay(100);
  digitalWrite(STAOFF, 1);
  delay(100);
  digitalWrite(STAOFF, 0);
  delay(100);
  digitalWrite(STAOFF, 1);
}

// 按钮1数据处理回调函数，开关状态取反
void Button1Callback(const String &state)
{
  if (state == BLINKER_CMD_ON)
  {
    BLINKER_LOG("Toggle on!");
    Button1.icon("fas fa-lightbulb");
    Button1.print("on");
    serve_enable(1);
    myservo.write(zero_angle - angle - 5); // tell servo to go to position in variable 'pos'
    delay(300);
    myservo.write(zero_angle - 1);
    delay(100);
    serve_enable(0);
  }
  else if (state == BLINKER_CMD_OFF)
  {
    BLINKER_LOG("Toggle off!");
    Button1.icon("far fa-lightbulb");
    Button1.print("off");
    serve_enable(1);
    myservo.write(zero_angle + angle + 5); // tell servo to go to position in variable 'pos'
    delay(300);
    myservo.write(zero_angle + 1);
    delay(100);
    serve_enable(0);
  }
}

// 按钮2数据处理回调函数，断开总电源
void Button2Callback(const String &state)
{
  if (state == BLINKER_CMD_BUTTON_TAP)
  {
    BLINKER_LOG("Button tap!");
    Button1.text("断电");
    PowerOFF();
  }
}

// 自定义心跳函数，返回开关状态
void heartbeat()
{
  if (myservo.read() > zero_angle)
    Text1.print("灯开");
  else
    Text1.print("灯关");
}

// 检测收到未解析数据时的回调函数
void dataRead(const String &data)
{
  BLINKER_LOG("Blinker readString: ", data);
  int d_flag = data.toInt();
  if (d_flag == 1) // 发送数字为1，清除联网信息
  {
    wifiManager.resetSettings();
    Blinker.print("resetSettings and reset ESP");
    ESP.reset();
  }
  Blinker.vibrate();
  uint32_t BlinkerTime = millis();
  Blinker.print("millis", BlinkerTime);
  digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
}

void setup()
{
  WiFi.setSleepMode(WIFI_LIGHT_SLEEP, 250); // 设置睡眠模式（可选参数：WIFI_NONE_SLEEP、WIFI_LIGHT_SLEEP、WIFI_MODEM_SLEEP ）
  myservo.attach(PWM1);                     // PWM1
  myservo.write(zero_angle);
  Serial.begin(115200); // 初始化串口服务
  BLINKER_DEBUG.stream(Serial);
  pinMode(BUILTIN_LED, OUTPUT); // 指示灯初始状态
  digitalWrite(BUILTIN_LED, LOW);
  pinMode(serveSwitch, OUTPUT); // 舵机初始状态
  digitalWrite(serveSwitch, LOW);
  pinMode(STAOFF, OUTPUT); // ip5306常开
  digitalWrite(STAOFF, HIGH);
  init_littlefs();
  init_wifiManager();

  // Binker设备配置
  Serial.println(auth);
  Serial.println(ssid);
  Serial.println(pswd);
  Blinker.begin(auth, ssid, pswd);

  // 注册Blinker APP命令的回调函数
  Blinker.attachData(dataRead);
  Button1.attach(Button1Callback); // 按钮1回调函数注册
  Button2.attach(Button2Callback); // 按钮2回调函数注册
  Blinker.attachHeartbeat(heartbeat);
  timer2.attach(30, flag_change);
}

void loop()
{
  if (flag)
  {
    Serial.println(auth);
    Serial.println(ssid);
    Serial.println(pswd);
    // 检测blinker的auth值是否正确，正确blinker初始化成功，Blinker.init()值不会在一个循环内刷新，故放置在10s循环内
    if (!Blinker.init())
    {
      // wifiManager.resetSettings(); // reset saved settings
      // ESP.reset();
    }
    // 检测运行是否断网，WiFi.status()值不会立即刷新
    if (WiFi.status() == WL_CONNECTED)
      digitalWrite(LED_BUILTIN, HIGH);
    else
    {
      digitalWrite(LED_BUILTIN, LOW);
      // wifiManager.resetSettings(); // reset saved settings
      // ESP.reset();
    }
    flag = 0;
  }
  Blinker.run();
}
