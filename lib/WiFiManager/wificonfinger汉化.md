修改lib\WiFiManager\wm_strings_en.h中的PROGMEM字段后，上传后没有效果，经测试，将lib\WiFiManager\WiFiManager.h的    #define WM_DEBUG_LEVEL DEBUG_NOTIFY 的DEBUG_NOTIFY修改为其他级别的（例如DEBUG_ERROR），改为高级别的debug会导致重启，
这种情况猜测是esp8266的flash擦除问题。
```
const char HTTP_FORM_END[]         PROGMEM = "<br/><br/><button onclick='OnLikeClicked()' type='submit'>保存</button><script>function OnLikeClicked(){alert(\"成功\")}</script></form>";

```