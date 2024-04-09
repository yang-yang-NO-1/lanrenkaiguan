# 懒人开关舵机版
![](/readme/2024-03-10-18-46-17.png)
使用点灯科技APP控制，界面配置如下
```
{¨config¨{¨headerColor¨¨transparent¨¨headerStyle¨¨light¨¨background¨{¨img¨¨assets/img/headerbg.jpg¨¨isFull¨«}}¨dashboard¨|{¨type¨¨deb¨¨mode¨É¨bg¨É¨cols¨Ñ¨rows¨Ì¨key¨¨debug¨´x´É´y´¤C}{ßA¨btn¨¨ico¨¨fal fa-power-off¨ßCÊ¨t0¨¨开关灯¨¨t1¨¨文本2¨ßDÉßEÍßFÍßG¨PWM1¨´x´Ë´y´Ï¨speech¨|÷¨lstyle¨Ë}{ßA¨tex¨ßL´´ßN¨开关状态¨¨size¨¤EßDÉßJ¨fal fa-lightbulb¨ßEÍßFËßG¨T1¨´x´Ë´y´ÌßQ|÷ßRÌ¨clr¨¨#076EEF¨}{ßA¨inp¨ßDÉßEÑßFËßG¨inp-6yh¨´x´É´y´É}÷¨actions¨|¦¨cmd¨¦¨switch¨‡¨text¨‡¨on¨¨打开?name¨¨off¨¨关闭?name¨—÷¨triggers¨|{¨source¨ßd¨source_zh¨ßT¨state¨|ßfßh÷¨state_zh¨|¨打开¨¨关闭¨÷}÷}
```
## 使用说明
1.打开点灯科技app，添加一个独立设备，将以上界面配置粘贴至该设备的界面配置中，点击更新配置即可自动添加控制按钮等界面，复制该设备的秘钥以备接下来使用。
2.连接名为懒人开关的wifi热点，自动弹出网络配置界面，点击要连接的wifi名称，输入密码，将秘钥填入auth框内，点击确认，等待蓝色板载指示灯灭表示已经连接至wifi网络。
3.打开点灯APP，点击按钮即可控制舵机打开或关闭开关。
## 硬件说明
sw拨动开关为电量指示灯开关，减少设备耗电同时防止灯光污染，sw1按键为上电开关，单击即可为打开电源输出，双击即可关闭电源输出，flash按键为程序烧录按键。
```
#define PWM1 5        // 舵机驱动引脚
#define zero_angle 95 // 舵机零点角度
#define angle 20      // 舵机动作角度

void Button1Callback(const String &state)
{
  if (state == BLINKER_CMD_ON)
  {
    BLINKER_LOG("Toggle on!");
    Button1.icon("fas fa-lightbulb");
    Button1.print("on");
    myservo.write(zero_angle - angle - 5); // tell servo to go to position in variable 'pos'
    delay(500);
    myservo.write(zero_angle - 1);
  }
  else if (state == BLINKER_CMD_OFF)
  {
    BLINKER_LOG("Toggle off!");
    Button1.icon("far fa-lightbulb");
    Button1.print("off");
    myservo.write(zero_angle + angle + 5); // tell servo to go to position in variable 'pos'
    delay(500);
    myservo.write(zero_angle + 1);
  }
}

```
# esp8266做远程开关功耗太高，故添加远程断开控制板电源，手动开启的方式