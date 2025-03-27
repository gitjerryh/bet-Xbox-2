#include <arduino.h>
#include "WebPage.h"
// Replace with your network credentials
// WIFI_AP settings.
const char* AP_SSID = "WAVESHARE Robot";
const char* AP_PWD  = "1234567890";

// WIFI_STA settings.
const char* STA_SSID = "OnePlus 10 Pro";
const char* STA_PWD  = "55132768";
// set the default wifi mode here.
// 1 as [AP] mode, it will not connect other wifi.
// also, 1 as upper computer control mode.
// 2 as [STA] mode, it will connect to know wifi.
#define DEFAULT_WIFI_MODE 2
extern int WIFIP_MODE;

// 更新为使用String类型
extern String IP_ADDRESS_STR;
extern uint8_t WIFI_MODE;
extern String MAC_ADDRESS;
extern int WIFI_RSSI;

#include "esp_wifi.h"
#include <WiFi.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

// 恢复HTTP服务器相关的头文件
#include <WebServer.h>  // 使用ESP32的WebServer库代替esp_http_server

// 注释掉相机相关的头文件，如果不需要相机功能
// #include "dl_lib_matrix3d.h"
// #include <esp32-hal-ledc.h>
// #include "esp_http_server.h"
// #include "esp_timer.h"
// #include "esp_camera.h"
// #include "img_converters.h"

// 注释掉相机模型定义
// #define CAMERA_MODEL_ESP_EYE
// #define PWDN_GPIO_NUM     -1
// #define RESET_GPIO_NUM    -1
// #define XCLK_GPIO_NUM     4
// #define SIOD_GPIO_NUM     18
// #define SIOC_GPIO_NUM     23

// #define Y9_GPIO_NUM       36
// #define Y8_GPIO_NUM       37
// #define Y7_GPIO_NUM       38
// #define Y6_GPIO_NUM       39
// #define Y5_GPIO_NUM       35
// #define Y4_GPIO_NUM       14
// #define Y3_GPIO_NUM       13
// #define Y2_GPIO_NUM       34

extern int MiddlePosition;
extern int moveFB;
extern int moveLR;
extern int debugMode;
extern int funcMode;
extern void initPosAll();
extern void middlePosAll();
extern void servoDebug(byte servoID, int offset);
extern void servoConfigSave(byte activeServo);
extern int ServoMiddlePWM[16];
extern int CurrentPWM[16];

extern int GAIT_TYPE;
extern int servoSPEED;
extern int gestureSpeedCtrl;
extern float gestureUD;
extern float gestureLR;
extern float gestureOffSetMax;
extern float gestureSpeed;

// 创建WebServer实例
WebServer server(80);

// 函数实现
extern void getMAC(){
  MAC_ADDRESS = WiFi.macAddress();
}

extern void getIP(){
  if(WIFI_MODE == 1){ 
    IP_ADDRESS_STR = WiFi.softAPIP().toString(); 
  }
  else if(WIFI_MODE == 2 || WIFI_MODE == 3){ 
    IP_ADDRESS_STR = WiFi.localIP().toString(); 
  }
}

void setAP(){
  WiFi.softAP(AP_SSID, AP_PWD);
  IPAddress myIP = WiFi.softAPIP();
  IP_ADDRESS_STR = myIP.toString();
  Serial.print("AP IP address: ");
  Serial.println(myIP);
}

void setSTA(){
  WIFI_MODE = 3;
  WiFi.begin(STA_SSID, STA_PWD);
}

extern void getWifiStatus(){
  if(WiFi.status() == WL_CONNECTED){
    WIFI_MODE = 2;
    getIP();
    WIFI_RSSI = WiFi.RSSI();
  }
  else if(WiFi.status() == WL_CONNECTION_LOST && DEFAULT_WIFI_MODE == 2){
    WIFI_MODE = 3;
    // WiFi.disconnect();
    WiFi.reconnect();
  }
}

void wifiInit(){
  WIFI_MODE = DEFAULT_WIFI_MODE;
  if(WIFI_MODE == 1){setAP();}
  else if(WIFI_MODE == 2){setSTA();}
}

// 处理根路径请求
void handleRoot() {
  server.send(200, "text/html", INDEX_HTML);
}

// 处理机器人控制命令
void handleControl() {
  String var = server.arg("var");
  String val = server.arg("val");
  
  int value = val.toInt();
  
  Serial.print("Command: var=");
  Serial.print(var);
  Serial.print(", val=");
  Serial.println(value);
  
  if(var == "FB") {
    if(value == 1) moveFB = 1;
    else if(value == 2) moveFB = -1;
    else if(value == 3) moveFB = 0;
  }
  else if(var == "LR") {
    if(value == 1) moveLR = 1;
    else if(value == 2) moveLR = -1;
    else if(value == 3) moveLR = 0;
  }
  else if(var == "debug") {
    debugMode = value;
  }
  else if(var == "func") {
    funcMode = value;
  }
  else if(var == "initialPOS") {
    initPosAll();
  }
  else if(var == "middlePOS") {
    middlePosAll();
  }
  else if(var == "gait") {
    GAIT_TYPE = value;
  }
  else if(var == "speedCtrl") {
    // value is 1 as speed++, 2 as speed--, 3 as speed
    if(value == 1) servoSPEED = servoSPEED + 10;
    else if(value == 2) servoSPEED = servoSPEED - 10;
    else servoSPEED = value;
  }
  else if(var == "gestureSpeedCtrl") {
    gestureSpeedCtrl++;
    if(gestureSpeedCtrl > 3) gestureSpeedCtrl = 1;
    if(gestureSpeedCtrl == 1) gestureSpeed = 0.5;
    else if(gestureSpeedCtrl == 2) gestureSpeed = 1;
    else if(gestureSpeedCtrl == 3) gestureSpeed = 2;
  }
  else if(var == "servoDebug") {
    servoDebug((byte)value, 0);
  }
  else if(var == "servoDebugInc") {
    CurrentPWM[value] = CurrentPWM[value] + 1;
    servoDebug((byte)value, CurrentPWM[value]);
  }
  else if(var == "servoDebugDec") {
    CurrentPWM[value] = CurrentPWM[value] - 1;
    servoDebug((byte)value, CurrentPWM[value]);
  }
  else if(var == "servoConfigSave") {
    servoConfigSave((byte)value);
  }
  // 添加处理舵机配置请求的处理
  else if(var == "sconfig") {
    int cmd = server.arg("cmd").toInt();
    if(cmd > 0) {
      // 增加PWM
      Serial.print("增加PWM");
      Serial.println(value);
      CurrentPWM[value] = CurrentPWM[value] + 1;
      servoDebug((byte)value, CurrentPWM[value]);
    } else {
      // 减少PWM
      Serial.print("减少PWM");
      Serial.println(value);
      CurrentPWM[value] = CurrentPWM[value] - 1;
      servoDebug((byte)value, CurrentPWM[value]);
    }
  }
  else if(var == "sset") {
    // 保存舵机配置
    Serial.print("保存舵机配置");
    Serial.println(value);
    servoConfigSave((byte)value);
  }
  else if(var == "move") {
    debugMode = 0;
    funcMode = 0;
    
    // 增加网页控制请求来源标识，确保不被Xbox控制逻辑覆盖
    extern bool webControlActive;
    webControlActive = true;
    
    if(value == 1) {
      Serial.println("Forward");
      moveFB = 1;
    }
    else if(value == 2) {
      Serial.println("TurnLeft");
      moveLR = -1;
    }
    else if(value == 3) {
      Serial.println("FBStop");
      moveFB = 0;
    }
    else if(value == 4) {
      Serial.println("TurnRight");
      moveLR = 1;
    }
    else if(value == 5) {
      Serial.println("Backward");
      moveFB = -1;
    }
    else if(value == 6) {
      Serial.println("LRStop");
      moveLR = 0;
    }
    
    // 设置一个短暂的延时，确保网页控制信号有效，然后允许Xbox控制器接管
    extern unsigned long webControlActiveTime;
    webControlActiveTime = millis();
  }
  else if(var == "funcMode") {
    debugMode = 0;
    if(value == 1) {
      if(funcMode == 1) {
        funcMode = 0;
        Serial.println("Steady OFF");
      }
      else if(funcMode == 0) {
        funcMode = 1;
        Serial.println("Steady ON");
      }
    }
    else {
      funcMode = value;
      Serial.println(value);
    }        
  }
  
  server.send(200, "text/plain", "OK");
}

// 设置并启动Web服务器
void startWebServer() {
  server.on("/", handleRoot);
  server.on("/control", handleControl);
  
  server.begin();
  Serial.println("HTTP server started");
}

// Web服务器初始化函数
void webServerInit() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); // prevent brownouts by silencing them
  
  // 只保留WiFi初始化部分
  wifiInit();
  delay(1000);
  
  // 启动Web服务器
  startWebServer();
}
