#include <WiFiManager.h>
#include "Ticker.h"
#include <LittleFS.h>
#include "..\lib\blinker-library-master\src\modules\ArduinoJson\ArduinoJson.h"

bool flag; // 刷新标志
bool shouldSaveConfig = false;
char auth[20] = "";
char ssid[20] = "";
char pswd[20] = "";
WiFiManager wifiManager;

void flag_change()
{
  flag = 1;
}

void init_littlefs()
{
  if (LittleFS.begin())
  {
    // if file not exists
    if (!(LittleFS.exists("/AC.json")))
    {
      LittleFS.open("/AC.json", "w+");
      return;
    }

    File configFile = LittleFS.open("/AC.json", "r");
    if (configFile)
    {
      String a;
      StaticJsonDocument<200> doc;
      while (configFile.available())
      {
        a = configFile.readString();
      }
      Serial.println("");
      Serial.println(a);
      Serial.println(a);
      configFile.close();
      deserializeJson(doc, a);

      if (doc.containsKey("auth"))
      {
        strcpy(auth, doc["auth"]);
      }
      if (doc.containsKey("ssid"))
      {
        strcpy(ssid, doc["ssid"]);
      }
      if (doc.containsKey("pswd"))
      {
        strcpy(pswd, doc["pswd"]);
      }
    }
    configFile.close();
  }
  else
  {
    Serial.println("LittleFS mount failed");
    return;
  }
}

bool saveConfig()
{
  File configFile = LittleFS.open("/AC.json", "r");
  if (!configFile)
  {
    Serial.println("failed to open config file for writing");
    return false;
  }
  configFile.close();
  StaticJsonDocument<200> doc;
  doc["auth"] = auth;
  doc["ssid"] = ssid;
  doc["pswd"] = pswd;
  File fileSaved = LittleFS.open("/AC.json", "w");
  serializeJson(doc, fileSaved);
  serializeJson(doc, Serial);
  Serial.println(fileSaved);
  fileSaved.close();
  // end save
  return true;
}
// 保存wifi信息时的回调函数
void STACallback()
{
  digitalWrite(LED_BUILTIN, HIGH);
  Serial.println("connected wifi !!!");
  shouldSaveConfig = true;
}
// 设置AP模式时的回调函数
void APCallback(WiFiManager *myWiFiManager)
{
  digitalWrite(LED_BUILTIN, LOW);
  Serial.println("NO connected !!!");
}

void init_wifiManager()
{
  wifiManager.setTitle("Wish today");
  WiFiManagerParameter blinker_auth("auth", "blinker_auth(12位)", "", 13);
  WiFiManagerParameter host_hint("<small> Blinker_auth <br></small><br><br>");
  WiFiManagerParameter p_lineBreak_notext("<p></p>");
  wifiManager.setSaveConfigCallback(STACallback);
  wifiManager.setAPCallback(APCallback);
  // 配置连接超时，单位秒
  wifiManager.setConnectTimeout(10);
  // 设置 如果配置错误的ssid或者密码 退出配置模式
  wifiManager.setBreakAfterConfig(true);
  wifiManager.addParameter(&p_lineBreak_notext);
  // wifiManager.addParameter(&host_hint);
  wifiManager.addParameter(&blinker_auth);

  // wifiManager.resetSettings(); //reset saved settings
  if (!wifiManager.autoConnect("懒人开关"))
  {
    // reset and try again, or maybe put it to deep sleep
    ESP.reset();
  }
  if (shouldSaveConfig)
  {
    int ssid_len = wifiManager.getWiFiSSID().length() + 1;
    int pswd_len = wifiManager.getWiFiPass().length() + 1;
    wifiManager.getWiFiSSID().toCharArray(ssid, ssid_len);
    wifiManager.getWiFiPass().toCharArray(pswd, pswd_len);
    Serial.println(wifiManager.getWiFiSSID());
    Serial.println(wifiManager.getWiFiPass());
    Serial.println(ssid);
    Serial.println(pswd);
    strcpy(auth, blinker_auth.getValue());
    saveConfig();
    // ESP.reset();
  }
}