// WIFI settings in app_httpd.cpp
#include <Arduino.h>  // 添加Arduino核心库
#include <WiFi.h>     // 添加ESP32的WiFi库
#include <WebServer.h> // 添加WebServer库
// 移除PS3控制器库，添加Xbox控制器库
#include <XboxSeriesXControllerESP32_asukiaaa.hpp>
// 保留蓝牙和NVS相关头文件
#include <nvs_flash.h>
#include <esp_bt.h>
#include <esp_bt_main.h>
#include <esp_bt_device.h>
#include <esp_gap_bt_api.h>
#include <esp_err.h>
#include <esp_log.h>

// OLED screen display.
// === PAGE 1 ===
// Row1: [WIFI_MODE]:IP ADDRESS
//       [WIFI_MODE]: 1 as [AP] mode, it will not connect other wifi.
//                    2 as [STA] mode, it will connect to known wifi.
// Row2: [RSSI]
// Row3: [STATUS] [A] [B] [C] [D]
//       [A]: 1 as forward. -1 as backward.
//       [B]: 1 as turn right. -1 as turn left.
//       [C]: 0 as not in the debugMode. Robot can be controled.
//            1 as in the debugMode. You can debug the servos.
//       [D]: 0 as not in any function mode. Robot can be controled to move around.
//            1 as in steady mode. keep balancing.
//            2 as stayLow action.
//            3 as handshake action.
//            4 as jump action.
//            5, 6 and 7 as ActionA, ActionB and ActionC.
//            8 as servos moving to initPos, initPos is the middle angle for servos.
//            9 as servos moving to middlePos, middlePos is the middle angle for program.
// Row4: [BATTERY]
// === PAGE 2 ===
// [SHOW] DebugMode via wire config.
// [ . . . o o ]  LED G21 G15 G12 3V3
// [ . . . . . ]  TX  RX  GND  5V  5V
//    <SWITCH>

// 定义函数原型
void setup();
void loop();
void robotThreadings(void *pvParameter);
void threadingsInit();
void serialCtrl();
void jsonSend();
void getWifiStatus();
void processXboxInput();
void onXboxConnect();
void onXboxDisconnect();
void setupXboxController();
void checkXboxStatus();
void printDeviceAddress();
void periodicStateReset();
void robotCtrl();

// 全局变量
String IP_ADDRESS_STR = "0.0.0.0";  // 使用String代替IPAddress
uint8_t WIFI_MODE = 0; // select WIFI_MODE in app_httpd.cpp
void getWifiStatus();
int WIFI_RSSI = 0;

// gait type ctrl
// 0: simpleGait(DiagonalGait).
// 1: triangularGait.
int GAIT_TYPE = 0;

int CODE_DEBUG = 0;

// 添加Xbox控制器对象和状态变量
XboxSeriesXControllerESP32_asukiaaa::Core xboxController("00:00:00:00:00:00"); // 默认MAC地址，后续会更新
bool XBOX_CONNECTED = false;
unsigned long lastXboxStatusCheck = 0;
unsigned long lastXboxConnectAttempt = 0;
int xboxConnectAttempts = 0;
const int XBOX_RECONNECT_INTERVAL = 10000; // 10秒重连间隔
const int MAX_XBOX_CONNECT_ATTEMPTS = 5;   // 最大重连次数，之后将等待更长时间

// 添加Xbox控制器摇杆校准变量
bool firstReading = true;  // 控制是否进行摇杆校准
int16_t centerLX = 32767;  // 摇杆中心值
int16_t centerLY = 32767;
int16_t centerRX = 32767;
int16_t centerRY = 32767;

// 添加Xbox按钮长按复位相关变量
unsigned long xboxXButtonPressTime = 0;  // Xbox按钮按下时间
bool xboxResetPerformed = false;  // 是否已执行复位

// 添加网页控制相关变量
bool webControlActive = false;  // 标识网页控制活动状态
unsigned long webControlActiveTime = 0;  // 网页控制活动时间
const unsigned long WEB_CONTROL_TIMEOUT = 1000;  // 网页控制超时时间(毫秒)

// ctrl interface.
// refer to OLED screen display for more detail information.
int moveFB = 0;
int moveLR = 0;
int debugMode = 0;
int funcMode  = 0;
float gestureUD = 0;
float gestureLR = 0;
float gestureOffSetMax = 15;
float gestureSpeed = 2;
int STAND_STILL = 0;
int servoSPEED = 100; // 定义servoSPEED变量，初始值设为100
int gestureSpeedCtrl = 2; // 定义gestureSpeedCtrl变量，初始值设为2（对应gestureSpeed=1）

const char* UPPER_IP = "";
int UPPER_TYPE = 0;
unsigned long LAST_JSON_SEND;
int JSON_SEND_INTERVAL;

// 在InitConfig.h中声明为extern的变量的定义
int MiddlePosition = 300;
int CurrentPWM[16] = {300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300};

// import libraries.
#include "InitConfig.h"
#include "ServoCtrl.h"
#include "PreferencesConfig.h"
#include <ArduinoJson.h>

// 定义一个单独的函数来手动前向声明和导入所有库
void importLibrariesAndFunctions() {
  // 这个函数不会被调用，仅用于编译器识别
  extern void periodicStateReset();
}

StaticJsonDocument<200> docReceive;
StaticJsonDocument<100> docSend;
TaskHandle_t threadings;

// placeHolders.
void webServerInit();

// 减少内存使用的常量字符串，使用PROGMEM存储
const char SERIAL_INIT[] PROGMEM = "Serial initialized";
const char I2C_INIT[] PROGMEM = "I2C initialized";
const char WIRE_DEBUG_INIT[] PROGMEM = "Wire debug initialized";
const char INA219_INIT[] PROGMEM = "INA219 initialized";
const char ICM20948_INIT[] PROGMEM = "ICM20948 initialized";
const char BUZZER_INIT[] PROGMEM = "Buzzer initialized";
const char RGB_INIT[] PROGMEM = "RGB initialized";
const char SERVO_INIT[] PROGMEM = "Servo setup completed";
const char SCREEN_INIT[] PROGMEM = "Screen initialized";
const char PREFERENCES_INIT[] PROGMEM = "Preferences setup completed";
const char XBOX_INIT[] PROGMEM = "Xbox controller initialized";

// var(variable), val(value).                  
void serialCtrl(){
  if (Serial.available()){
    // Read the JSON document from the "link" serial port
    DeserializationError err = deserializeJson(docReceive, Serial);

    if (err == DeserializationError::Ok){
      UPPER_TYPE = 1;
      int val = docReceive["val"].as<int>();

      if(docReceive["var"] == "funcMode"){
        debugMode = 0;
        gestureUD = 0;
        gestureLR = 0;
        if(val == 1){
          if(funcMode == 1){funcMode = 0;}
          else if(funcMode == 0){funcMode = 1;}
        }
        else{
          funcMode = val;
        }
      }

      else if(docReceive["var"] == "move"){
        debugMode = 0;
        funcMode  = 0;
        digitalWrite(BUZZER, HIGH);
        switch(val){
          case 1: moveFB = 1; break;
          case 2: moveLR =-1; break;
          case 3: moveFB = 0; break;
          case 4: moveLR = 1; break;
          case 5: moveFB =-1; break;
          case 6: moveLR = 0; break;
        }
      }

      else if(docReceive["var"] == "ges"){
        debugMode = 0;
        funcMode  = 0;
        switch(val){
          case 1: gestureUD += gestureSpeed;if(gestureUD > gestureOffSetMax){gestureUD = gestureOffSetMax;}break;
          case 2: gestureUD -= gestureSpeed;if(gestureUD <-gestureOffSetMax){gestureUD =-gestureOffSetMax;}break;
          case 3: break;
          case 4: gestureLR -= gestureSpeed;if(gestureLR <-gestureOffSetMax){gestureLR =-gestureOffSetMax;}break;
          case 5: gestureLR += gestureSpeed;if(gestureLR > gestureOffSetMax){gestureLR = gestureOffSetMax;}break;
          case 6: break;
        }
        pitchYawRollHeightCtrl(gestureUD, gestureLR, 0, 0);
      }

      else if(docReceive["var"] == "light"){
        switch(val){
          case 0: setSingleLED(0,matrix.Color(0, 0, 0));setSingleLED(1,matrix.Color(0, 0, 0));break;
          case 1: setSingleLED(0,matrix.Color(0, 32, 255));setSingleLED(1,matrix.Color(0, 32, 255));break;
          case 2: setSingleLED(0,matrix.Color(255, 32, 0));setSingleLED(1,matrix.Color(255, 32, 0));break;
          case 3: setSingleLED(0,matrix.Color(32, 255, 0));setSingleLED(1,matrix.Color(32, 255, 0));break;
          case 4: setSingleLED(0,matrix.Color(255, 255, 0));setSingleLED(1,matrix.Color(255, 255, 0));break;
          case 5: setSingleLED(0,matrix.Color(0, 255, 255));setSingleLED(1,matrix.Color(0, 255, 255));break;
          case 6: setSingleLED(0,matrix.Color(255, 0, 255));setSingleLED(1,matrix.Color(255, 0, 255));break;
          case 7: setSingleLED(0,matrix.Color(255, 64, 32));setSingleLED(1,matrix.Color(32, 64, 255));break;
        }
      }

      else if(docReceive["var"] == "buzzer"){
        digitalWrite(BUZZER, val ? LOW : HIGH);
      }
    }
    else {
      while (Serial.available() > 0)
        Serial.read();
    }
  }
}

void jsonSend(){
  if(millis() - LAST_JSON_SEND > JSON_SEND_INTERVAL || millis() < LAST_JSON_SEND){
    docSend["vol"] = loadVoltage_V;
    serializeJson(docSend, Serial);
    LAST_JSON_SEND = millis();
  }
}

void robotThreadings(void *pvParameter){
  delay(3000);
  while(1){
    serialCtrl();
    xboxController.onLoop(); // 处理Xbox控制器事件
    if (xboxController.isConnected() && !xboxController.isWaitingForFirstNotification()) {
      processXboxInput(); // 处理Xbox输入
    }
    checkXboxStatus(); // 定期检查Xbox状态
    delay(25);
  }
}

void threadingsInit(){
  xTaskCreate(&robotThreadings, "RobotThreadings", 4000, NULL, 5, &threadings);
}

void setup() {
  // 首先初始化串口
  Serial.begin(115200);
  Serial.println(SERIAL_INIT);
  
  // 然后初始化I2C
  Wire.begin(S_SDA, S_SCL);
  Wire.setClock(400000);
  Serial.println(I2C_INIT);
  delay(200);
  
  // 初始化各个组件
  wireDebugInit();
  Serial.println(WIRE_DEBUG_INIT);
  
  InitINA219();
  Serial.println(INA219_INIT);
  
  InitICM20948();
  Serial.println(ICM20948_INIT);
  
  InitBuzzer();
  Serial.println(BUZZER_INIT);
  
  InitRGB();
  Serial.println(RGB_INIT);
  
  ServoSetup();
  Serial.println(SERVO_INIT);
  
  InitScreen();
  Serial.println(SCREEN_INIT);
  
  preferencesSetup();
  Serial.println(PREFERENCES_INIT);

  // 初始化机器人站立姿态
  delay(200);
  setSingleLED(0,matrix.Color(0, 128, 255));
  setSingleLED(1,matrix.Color(0, 128, 255));
  standMassCenter(0, 0);
  GoalPosAll();
  delay(1000);
  setSingleLED(0,matrix.Color(255, 128, 0));
  setSingleLED(1,matrix.Color(255, 128, 0));
  delay(500);

  // WEBCTRL INIT
  webServerInit();
  
  // 初始化Xbox控制器
  setupXboxController();
  Serial.println(XBOX_INIT);
  
  // RGB LEDs on
  delay(500);
  setSingleLED(0,matrix.Color(0, 32, 255));
  setSingleLED(1,matrix.Color(255, 32, 0));

  // 更新屏幕数据
  allDataUpdate();

  // 启动线程
  threadingsInit();
}

// main loop.
void loop() {
  // 处理Web服务器请求
  extern WebServer server;
  server.handleClient();
  
  // 每隔一段时间更新WiFi状态
  static unsigned long lastWifiCheck = 0;
  if (millis() - lastWifiCheck > 5000) {  // 每5秒检查一次
    getWifiStatus();
    lastWifiCheck = millis();
  }
  
  // 机器人控制
  robotCtrl();
  
  // 添加定期状态复位功能
  periodicStateReset();
  
  allDataUpdate();
  wireDebugDetect();
}

// 添加周期性状态复位功能，防止长时间操作后的累积偏移
void periodicStateReset() {
  static unsigned long lastResetTime = 0;
  const unsigned long RESET_INTERVAL = 600000; // 修改为10分钟执行一次（原为5分钟）
  
  if (millis() - lastResetTime > RESET_INTERVAL) {
    // 只在非功能模式下执行，避免干扰特定功能
    if (funcMode == 0 && !debugMode && moveFB == 0 && moveLR == 0) {
      Serial.println("执行定期状态微复位");
      
      // 保存当前状态
      float currentGestureUD = gestureUD;
      float currentGestureLR = gestureLR;
      
      // 更平滑地逐步减小姿态偏移，而不是直接归零
      for (int i = 0; i < 5; i++) {
        gestureUD = currentGestureUD * (1.0 - (i * 0.2));
        gestureLR = currentGestureLR * (1.0 - (i * 0.2));
        
        // 应用姿态修正
        pitchYawRollHeightCtrl(gestureUD, gestureLR, 0, 0);
        GoalPosAll();
        delay(20); // 短暂延时让每一步都平滑执行
      }
      
      // 临时复位所有状态
      gestureUD = 0;
      gestureLR = 0;
      
      // 轻微提示用户复位完成，音量更低
      digitalWrite(BUZZER, LOW);
      delay(20); // 更短的蜂鸣声
      digitalWrite(BUZZER, HIGH);
      
      // 记录当前时间
      lastResetTime = millis();
    }
  }
}

// <<<<<<<<<<=== Devices on Board ===>>>>>>>>>>>>

// --- --- ---   --- --- ---   --- --- ---
// ICM20948 init. --- 9-axis sensor for motion tracking.
// InitICM20948();

// read and update the pitch, raw and roll data from ICM20948.
// accXYZUpdate();


// --- --- ---   --- --- ---   --- --- ---
// INA219 init. --- DC current/voltage sensor.
// InitINA219();

// read and update the voltage and current data from INA219.
// InaDataUpdate();


// --- --- ---   --- --- ---   --- --- ---
// RGB INIT. --- the 2 RGB LEDs in front of the robot. LED_NUM = 0, 1.
// InitRGB();

// control RGB LED. 0 <= R, G, B <= 255.
// setSingleLED(LED_NUM, matrix.Color(R, G, B));


// --- --- ---   --- --- ---   --- --- ---
// SSD1306 INIT. --- OLED Screen
// InitScreen();

// show the newest data on the OLED screen.
// for more information you can refer to <OLED screen display>.
// screenDataUpdate();


// --- --- ---   --- --- ---   --- --- ---
// BUZZER INIT. --- the device that make a sound.
// InitBuzzer();

// BUZZER on.
// digitalWrite(BUZZER, HIGH);

// BUZZER off.
// digitalWrite(BUZZER, LOW);


// --- --- ---   --- --- ---   --- --- ---
// PCA9685 INIT.
// ServoSetup();

// all servos move to the middle position of the servos.
// initPosAll();


// <<<<<<<<<<<<=== Servos/Legs/Motion Ctrl ===>>>>>>>>>>>>>>>

// --- --- ---   --- --- ---   --- --- ---
// all servos move to the middle position of the program. 
// the position that you have to debug it to make it moves to.
// middlePosAll();

// control a single servo by updating the angle data in GoalPWM[].
// once it is called, call GoalPosALL() to move all of the servos.
// goalPWMSet(servoNum, angleInput);

// all servos move to goal position(GoalPWM[]).
// GoalPosAll();

// Ctrl a single leg of WAVEGO, once it is called, call GoalPosALL() to move all of the servos.
// input (x,y) position and return angle alpha and angle beta.
//     O  X  O                 O ------ [BODY]      I(1)  ^  III(3)
//    /         .              |          |               |
//   /    |        O           |          |               |
//  O     y     .              |          |         II(2) ^  IV(4)
//   \.   |  .                 |          |
//    \.  .                    |          |
//     O  |                    |          |
//  .                          |          |
//   \.   |                    |          |
//    \-x-X                    X----z-----O
// ---------------------------------------------------------------
// x, y, z > 0
// singleLegCtrl(LEG_NUM, X, Y, Z);


// --- --- ---   --- --- ---   --- --- ---
// a simple gait to ctrl the robot.
// GlobalInput changes between 0-1.
// use directionAngle to ctrl the direction.
// once it is called, call GoalPosALL() to move all of the servos.
// simpleGait(GlobalInput, directionAngle);

// a triangular gait to ctrl the robot.
// GlobalInput changes between 0-1.
// use directionAngle to ctrl the direction.
// once it is called, call GoalPosALL() to move all of the servos.
// triangularGait(GlobalInput, directionAngle);


// --- --- ---   --- --- ---   --- --- ---
// Stand and adjust mass center.
//     ^
//     a
//     |
// <-b-M
// a,b > 0
// standMassCenter(aInput, bInput);


// --- --- ---   --- --- ---   --- --- ---
// ctrl pitch yaw and roll.
// pitchInput (-, +), if > 0, look up.
// yawInput   (-, +), if > 0, look right.
// rollInput  (-, +), if > 0, lean right.
// 75 < input < 115
// pitchYawRoll(pitchInput, yawInput, rollInput);


// --- --- ---   --- --- ---   --- --- ---
// balancing function.
// once it is called, call GoalPosALL() to move all of the servos.
// balancing();


// --- --- ---   --- --- ---   --- --- ---
// the default function to control robot.
// robotCtrl();


// <<<<<<<<<<<<<<<=== Save Data Permanently ===>>>>>>>>>>>>>>>>>>

// EEPROM INIT.
// preferencesSetup();

// save the current position of the servoNum in EEPROM.
// servoConfigSave(servoNum);

// read the saved middle position data of the servos from EEPROM.
// middleUpdate();

// 添加WebServer的extern声明
extern WebServer server;

// 定期检查Xbox控制器状态的函数
void checkXboxStatus() {
  static unsigned long lastStatusCheck = 0;
  
  // 每2秒检查一次连接状态
  if (millis() - lastStatusCheck > 2000) {
    bool currentStatus = xboxController.isConnected() && !xboxController.isWaitingForFirstNotification();
    
    // 状态发生变化
    if (currentStatus != XBOX_CONNECTED) {
      if (currentStatus) {
        Serial.println("Xbox控制器已连接");
        onXboxConnect();
      } else {
        Serial.println("Xbox控制器已断开");
        onXboxDisconnect();
      }
    }
    
    // 更新连接状态
    XBOX_CONNECTED = currentStatus;
    lastStatusCheck = millis();
  }
}

// Xbox控制器输入处理函数
void processXboxInput() {
  static const int16_t DEADZONE = 5000;  // 增大死区阈值
  
  // 如果网页控制活跃且未超时，暂时跳过Xbox控制器的移动控制
  if (webControlActive && (millis() - webControlActiveTime < WEB_CONTROL_TIMEOUT)) {
    // 只处理网页控制未覆盖的功能，如姿态控制和按钮
    
    // 使用右摇杆控制机械狗姿态
    int16_t rx = xboxController.xboxNotif.joyRHori;
    int16_t ry = xboxController.xboxNotif.joyRVert;
    
    // 首次读取时校准中心点位置
    if (firstReading) {
      firstReading = false;
      centerLX = xboxController.xboxNotif.joyLHori;
      centerLY = xboxController.xboxNotif.joyLVert;
      centerRX = rx;
      centerRY = ry;
      
      // 校准时不改变moveFB和moveLR值
      gestureUD = 0;
      gestureLR = 0;
      
      Serial.println("摇杆校准完成！");
      Serial.print("中心点: L(");
      Serial.print(centerLX);
      Serial.print(",");
      Serial.print(centerLY);
      Serial.print(") R(");
      Serial.print(centerRX);
      Serial.print(",");
      Serial.print(centerRY);
      Serial.println(")");
      return;
    }
    
    // 计算相对于中心点的偏移
    int16_t offsetRX = rx - centerRX;
    int16_t offsetRY = ry - centerRY;
    
    // 仅处理姿态控制，不处理移动
    if (abs(offsetRX) > DEADZONE || abs(offsetRY) > DEADZONE) {
      gestureUD = map(offsetRY, -32768, 32767, gestureOffSetMax, -gestureOffSetMax);
      gestureLR = map(offsetRX, -32768, 32767, -gestureOffSetMax, gestureOffSetMax);
      pitchYawRollHeightCtrl(gestureUD, gestureLR, 0, 0);
    }
  }
  else {
    // 网页控制不活跃或已超时，正常处理Xbox控制器输入
    webControlActive = false;  // 重置网页控制标志
    
    // 使用左摇杆控制前后左右移动
    int16_t lx = xboxController.xboxNotif.joyLHori;
    int16_t ly = xboxController.xboxNotif.joyLVert;
    int16_t rx = xboxController.xboxNotif.joyRHori;
    int16_t ry = xboxController.xboxNotif.joyRVert;
    
    // 首次读取时校准中心点位置
    if (firstReading) {
      firstReading = false;
      centerLX = lx;
      centerLY = ly;
      centerRX = rx;
      centerRY = ry;
      
      // 强制重置移动状态
      moveFB = 0;
      moveLR = 0;
      gestureUD = 0;
      gestureLR = 0;
      
      Serial.println("摇杆校准完成！");
      Serial.print("中心点: L(");
      Serial.print(centerLX);
      Serial.print(",");
      Serial.print(centerLY);
      Serial.print(") R(");
      Serial.print(centerRX);
      Serial.print(",");
      Serial.print(centerRY);
      Serial.println(")");
      return;
    }
    
    // 计算相对于中心点的偏移
    int16_t offsetLX = lx - centerLX;
    int16_t offsetLY = ly - centerLY;
    int16_t offsetRX = rx - centerRX;
    int16_t offsetRY = ry - centerRY;
    
    // 前后移动控制（使用相对值和死区）
    if (offsetLY < -DEADZONE) {
      moveFB = -1; // 修改为后退（原来是前进）
    } else if (offsetLY > DEADZONE) {
      moveFB = 1; // 修改为前进（原来是后退）
    } else {
      moveFB = 0; // 停止前后移动
    }
    
    // 左右转向控制（使用相对值和死区）
    if (offsetLX < -DEADZONE) {
      moveLR = 1; // 修改为右转（原来是左转）
    } else if (offsetLX > DEADZONE) {
      moveLR = -1; // 修改为左转（原来是右转）
    } else {
      moveLR = 0; // 停止左右转向
    }
    
    // 使用右摇杆控制机械狗姿态（俯仰和横滚）
    if (abs(offsetRX) > DEADZONE || abs(offsetRY) > DEADZONE) {
      // 将摇杆值映射到姿态控制范围
      gestureUD = map(offsetRY, -32768, 32767, gestureOffSetMax, -gestureOffSetMax);
      gestureLR = map(offsetRX, -32768, 32767, -gestureOffSetMax, gestureOffSetMax);
      
      // 应用姿态控制
      pitchYawRollHeightCtrl(gestureUD, gestureLR, 0, 0);
    }
  }
  
  // 处理按钮按下事件 - 始终处理，无论网页控制是否活跃
  static bool lastBtnA = false;
  static bool lastBtnB = false;
  static bool lastBtnX = false;
  static bool lastBtnY = false;
  static bool lastBtnLB = false;
  static bool lastBtnRB = false;
  static bool lastBtnXbox = false;
  static bool lastBtnStart = false;
  
  // A按钮 - 切换平衡模式
  if (xboxController.xboxNotif.btnA && !lastBtnA) {
    Serial.println("按下：A键");
    if(funcMode == 1) {
      funcMode = 0;
    } else {
      funcMode = 1;
    }
    debugMode = 0;
    gestureUD = 0;
    gestureLR = 0;
  }
  lastBtnA = xboxController.xboxNotif.btnA;
  
  // B按钮 - 执行握手动作
  if (xboxController.xboxNotif.btnB && !lastBtnB) {
    Serial.println("按下：B键");
    funcMode = 3;
    debugMode = 0;
  }
  lastBtnB = xboxController.xboxNotif.btnB;
  
  // Y按钮 - 执行跳跃动作
  if (xboxController.xboxNotif.btnY && !lastBtnY) {
    Serial.println("按下：Y键");
    funcMode = 4;
    debugMode = 0;
  }
  lastBtnY = xboxController.xboxNotif.btnY;
  
  // X按钮 - 执行蹲下动作
  if (xboxController.xboxNotif.btnX && !lastBtnX) {
    Serial.println("按下：X键");
    funcMode = 2;
    debugMode = 0;
  }
  lastBtnX = xboxController.xboxNotif.btnX;
  
  // Start按钮 - 重新校准摇杆
  if (xboxController.xboxNotif.btnStart && !lastBtnStart) {
    Serial.println("开始重新校准摇杆");
    firstReading = true;
    // 强制重置移动状态
    moveFB = 0;
    moveLR = 0;
    gestureUD = 0;
    gestureLR = 0;
  }
  lastBtnStart = xboxController.xboxNotif.btnStart;
  
  // LB按钮 - 切换步态类型
  if (xboxController.xboxNotif.btnLB && !lastBtnLB) {
    GAIT_TYPE = 0; // 简单步态
    Serial.println("切换到简单步态");
  }
  lastBtnLB = xboxController.xboxNotif.btnLB;
  
  // RB按钮 - 切换步态类型
  if (xboxController.xboxNotif.btnRB && !lastBtnRB) {
    GAIT_TYPE = 1; // 三角步态
    Serial.println("切换到三角步态");
  }
  lastBtnRB = xboxController.xboxNotif.btnRB;
  
  // 左右扳机控制速度
  if (xboxController.xboxNotif.trigLT > 15000) {
    if(STEP_ITERATE > 0.01) {
      STEP_ITERATE -= 0.01;
      Serial.print("减速: ");
      Serial.println(STEP_ITERATE);
    }
  }
  
  if (xboxController.xboxNotif.trigRT > 15000) {
    if(STEP_ITERATE < 0.1) {
      STEP_ITERATE += 0.01;
      Serial.print("加速: ");
      Serial.println(STEP_ITERATE);
    }
  }
  
  // Xbox按钮 - 复位
  if (xboxController.xboxNotif.btnXbox && !lastBtnXbox) {
    Serial.println("执行复位操作");
    // 记录按下时间，用于检测长按
    xboxXButtonPressTime = millis();
  }
  
  // 检测Xbox按钮长按，实现完全系统复位
  if (xboxController.xboxNotif.btnXbox) {
    if (millis() - xboxXButtonPressTime > 2000) { // 长按2秒进行完全复位
      if (!xboxResetPerformed) {
        Serial.println("执行完全系统复位");
        // 重置所有状态
        moveFB = 0;
        moveLR = 0;
        gestureUD = 0;
        gestureLR = 0;
        
        // 重置步态和功能模式
        GAIT_TYPE = 0;
        funcMode = 0;
        debugMode = 0;
        
        // 重置校准标志，以便重新校准
        firstReading = true;
        
        // 重置PID和平衡相关参数
        // 通过退出后重新进入steady模式来刷新平衡参数
        if (funcMode == 1) {
          funcMode = 0;
          delay(100);
          funcMode = 1;
        }
        
        // 重置所有腿部位置
        singleLegCtrl(1,  WALK_EXTENDED_X, STAND_HEIGHT, WALK_EXTENDED_Z);
        singleLegCtrl(2, -WALK_EXTENDED_X, STAND_HEIGHT, WALK_EXTENDED_Z);
        singleLegCtrl(3,  WALK_EXTENDED_X, STAND_HEIGHT, WALK_EXTENDED_Z);
        singleLegCtrl(4, -WALK_EXTENDED_X, STAND_HEIGHT, WALK_EXTENDED_Z);
        GoalPosAll();
        
        // 标记已执行重置
        xboxResetPerformed = true;
        
        // 提示用户重置完成
        digitalWrite(BUZZER, LOW);
        delay(200);
        digitalWrite(BUZZER, HIGH);
        delay(100);
        digitalWrite(BUZZER, LOW);
        delay(100);
        digitalWrite(BUZZER, HIGH);
      }
    }
  } else {
    // 按钮松开，重置标志
    xboxResetPerformed = false;
  }
  
  lastBtnXbox = xboxController.xboxNotif.btnXbox;
}

// Xbox控制器连接回调函数
void onXboxConnect() {
  XBOX_CONNECTED = true;
  
  Serial.println("Xbox控制器已连接!");
  
  // 设置LED指示连接成功
  setSingleLED(0, matrix.Color(0, 255, 0)); // 绿色LED表示Xbox已连接
  setSingleLED(1, matrix.Color(0, 255, 0));
  
  // 重置摇杆位置校准标志，在下一次processXboxInput时自动校准
  firstReading = true;
  
  // 防止误动作，初始化所有移动变量
  moveFB = 0;
  moveLR = 0;
  gestureUD = 0;
  gestureLR = 0;
  
  // 触发轻微震动反馈确认连接
  XboxSeriesXHIDReportBuilder_asukiaaa::ReportBase repo;
  repo.setAllOff();
  repo.v.select.center = true;
  repo.v.power.center = 10; // 10% power
  repo.v.timeActive = 10;   // 0.1 second
  xboxController.writeHIDReport(repo);
}

// Xbox控制器断开连接回调函数
void onXboxDisconnect() {
  XBOX_CONNECTED = false;
  
  Serial.println("Xbox控制器已断开连接!");
  
  // 设置LED指示断开连接
  setSingleLED(0, matrix.Color(255, 0, 0)); // 红色LED表示Xbox已断开
  setSingleLED(1, matrix.Color(255, 0, 0));
  delay(300);
  setSingleLED(0, matrix.Color(0, 32, 255));
  setSingleLED(1, matrix.Color(255, 32, 0));
}

// Xbox控制器初始化函数
void setupXboxController() {
  Serial.println("初始化Xbox控制器...");
  
  // 请将MAC地址替换为您实际的Xbox控制器MAC地址
  // 从用户提供的示例代码中获取MAC地址
  xboxController = XboxSeriesXControllerESP32_asukiaaa::Core("68:6c:e6:37:be:26");
  
  Serial.println("开始Xbox控制器连接...");
  xboxController.begin();
  
  // 显示连接信息
  Serial.println("\n========= Xbox控制器连接信息 =========");
  Serial.println("Xbox控制器MAC地址: 68:6c:e6:37:be:26");
  Serial.println("等待Xbox控制器连接...");
  Serial.println("=====================================\n");
  
  // 在OLED上显示
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);
  display.println("Xbox Controller");
  display.println("MAC: 68:6c:e6:37:be:26");
  display.println("Waiting for");
  display.println("connection...");
  display.display();
}

// 打印ESP32的MAC地址函数保留，以防需要
void printDeviceAddress() {
  Serial.println("\n========= ESP32蓝牙信息 =========");
  
  // 使用Arduino ESP32框架提供的btStart()函数初始化蓝牙
  if (!btStarted()) {
    Serial.println("初始化蓝牙...");
    
    // 初始化NVS
    Serial.println("初始化NVS...");
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      Serial.println("正在擦除NVS并重新初始化...");
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    
    // 使用Arduino ESP32的btStart()函数启动蓝牙
    Serial.println("启动蓝牙...");
    if (!btStart()) {
      Serial.println("btStart()函数调用失败，无法启动蓝牙");
      return;
    }
    
    // 初始化蓝牙栈
    Serial.println("初始化蓝牙栈...");
    esp_err_t result = esp_bluedroid_init();
    if (result != ESP_OK) {
      Serial.print("蓝牙栈初始化失败，错误码: ");
      Serial.println(result);
      return;
    }
    
    // 启用蓝牙栈
    Serial.println("启用蓝牙栈...");
    result = esp_bluedroid_enable();
    if (result != ESP_OK) {
      Serial.print("蓝牙栈启用失败，错误码: ");
      Serial.println(result);
      return;
    }
    
    Serial.println("蓝牙已成功初始化");
  } else {
    Serial.println("蓝牙已经初始化");
  }
  
  // 等待蓝牙初始化完成
  delay(1000);
  
  // 获取MAC地址
  const uint8_t* address = esp_bt_dev_get_address();
  if (address) {
    char str[18];
    sprintf(str, "%02X:%02X:%02X:%02X:%02X:%02X", 
            address[0], address[1], address[2], 
            address[3], address[4], address[5]);
            
    Serial.println("\n========= ESP32蓝牙MAC地址 =========");
    Serial.print("蓝牙MAC地址: ");
    Serial.println(str);
    Serial.println("=====================================\n");
    
    // 在OLED上显示
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0,0);
    display.println("ESP32 BT MAC:");
    display.println(str);
    display.display();
    delay(3000); // 显示3秒钟
  } else {
    Serial.println("无法获取ESP32蓝牙地址，请确认蓝牙已启用");
  }
}