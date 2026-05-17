// 示例片段：Slider + RGB + WS2812
// 这个文件是阅读示例，不参与 PlatformIO 默认编译。

#include <IotMiniBlinker.h>

extern IotMiniBlinker Iot;

void handleSliderBrightness(const IotMiniCommand& cmd, JsonObjectConst params) {
  int brightness = IotMiniBlinker::paramInt(params, "brightness", 0);
  brightness = constrain(brightness, 0, 255);
  Iot.number("num_brightness", brightness, "level");
  Iot.publishAck(cmd, true, "brightness ok");
}

void handleRGBColor(const IotMiniCommand& cmd, JsonObjectConst params) {
  String color = IotMiniBlinker::paramString(params, "color", "#000000");
  uint8_t r, g, b;
  if (IotMiniBlinker::parseColorHex(color, r, g, b)) {
    // analogWrite / LED strip driver here
    Iot.publishAck(cmd, true, "rgb ok");
  } else {
    Iot.publishAck(cmd, false, "invalid color");
  }
}

void handleWS2812(const IotMiniCommand& cmd, JsonObjectConst params) {
  String effect = IotMiniBlinker::paramString(params, "effect", "off");
  String color = IotMiniBlinker::paramString(params, "color", "#000000");
  int brightness = IotMiniBlinker::paramInt(params, "brightness", 0);
  // FastLED / Adafruit_NeoPixel driver here
  Iot.print("print_debug", String("WS2812 ") + effect + " " + color + " " + brightness);
  Iot.publishAck(cmd, true, "ws2812 ok");
}

void registerExample02() {
  Iot.attach("sli_brightness", handleSliderBrightness);
  Iot.attach("rgb_color", handleRGBColor);
  Iot.attach("ws2812_strip", handleWS2812);
}
