#ifndef INIT_CONFIG_H
#define INIT_CONFIG_H

#include <Wire.h>
#include <XboxSeriesXControllerESP32_asukiaaa.hpp>

// 添加外部变量引用
extern uint8_t WIFI_MODE;
extern String IP_ADDRESS_STR;  // 使用String替代IPAddress
extern int WIFI_RSSI;
extern int UPPER_TYPE;
extern const char* UPPER_IP;
extern int moveFB;
extern int moveLR;
extern int debugMode;
extern int funcMode;
extern void getWifiStatus();
extern bool XBOX_CONNECTED; // 添加Xbox控制器连接状态外部变量
extern XboxSeriesXControllerESP32_asukiaaa::Core xboxController; // 添加Xbox控制器对象引用
extern int GAIT_TYPE; // 步态类型
extern float STEP_ITERATE; // 移动速度步长

#ifndef TASK_HANDLE_DEFINED
#define TASK_HANDLE_DEFINED
TaskHandle_t dataUpdateHandle;
#endif

// select the core for OLED threading.
#if CONFIG_FREERTOS_UNICORE
// 仅在未定义时定义，避免重定义警告
#ifndef ARDUINO_RUNNING_CORE
#define ARDUINO_RUNNING_CORE 0
#endif
#define GAIT_RUNNING_CORE    1
#else
#define ARDUINO_RUNNING_CORE 1
#define GAIT_RUNNING_CORE    0
#endif

// Define GPIO.
#define S_SCL   33
#define S_SDA   32
#define RGB_LED 26
#define BUZZER  21
#define WIRE_DEBUG 14

// the middle position of the servos.
#ifndef MIDDLE_POSITION_DEFINED
#define MIDDLE_POSITION_DEFINED
extern int MiddlePosition;
#endif

// 
#ifndef CURRENT_PWM_DEFINED
#define CURRENT_PWM_DEFINED
extern int CurrentPWM[16];
#endif

// wire debug mode, P-wire, N-wire.
#define WIRE_DEBUG 12   // GPIO0

// WIRE_DEBUG: GPIO12------[D] LED
//                          [D] GND
//            <SWITCH>
// connect this two pins, and the robot go into debug mode.
#ifndef WIRE_DEBUG_INIT_DEFINED
#define WIRE_DEBUG_INIT_DEFINED
void wireDebugInit(){
  pinMode(WIRE_DEBUG, INPUT_PULLDOWN);
}
#endif

// <<<<<<<=====ICM20948: 0x68=========>>>>>>>>>>
#include <ICM20948_WE.h>
#define ICM20948_ADDR 0x68

#ifndef ACC_VARIABLES_DEFINED
#define ACC_VARIABLES_DEFINED
float ACC_X;
float ACC_Y;
float ACC_Z;
#endif

#ifndef MYIMU_DEFINED
#define MYIMU_DEFINED
ICM20948_WE myIMU = ICM20948_WE(ICM20948_ADDR);
#endif

#ifndef INIT_ICM20948_DEFINED
#define INIT_ICM20948_DEFINED
void InitICM20948(){
  // if(!myIMU.init()){
  //   Serial.println("ICM20948 does not respond");
  // }
  // else{
  //   Serial.println("ICM20948 is connected");
  // }
  // Serial.println("Position your ICM20948 flat and don't move it - calibrating...");
  myIMU.init();
  delay(200);
  myIMU.autoOffsets();
  // Serial.println("Done!"); 

  myIMU.setAccRange(ICM20948_ACC_RANGE_2G);
  myIMU.setAccDLPF(ICM20948_DLPF_6);
  myIMU.setAccSampleRateDivider(10);
}
#endif

#ifndef ACCXYZUPDATE_DEFINED
#define ACCXYZUPDATE_DEFINED
void accXYZUpdate(){
  xyzFloat accRaw;
  xyzFloat corrAccRaw;
  xyzFloat gVal;
  
  myIMU.readSensor();
  myIMU.getAccRawValues(&accRaw);
  myIMU.getCorrectedAccRawValues(&corrAccRaw);
  myIMU.getGValues(&gVal);

  ACC_X = corrAccRaw.x;
  ACC_Y = corrAccRaw.y;
  ACC_Z = corrAccRaw.z;
}
#endif

// <<<<<<<<<========INA219:0x42========>>>>>>>>
#include <INA219_WE.h>
#define INA219_ADDRESS 0x42

#ifndef INA219_DEFINED
#define INA219_DEFINED
INA219_WE ina219 = INA219_WE(INA219_ADDRESS);
#endif

#ifndef INA219_VARIABLES_DEFINED
#define INA219_VARIABLES_DEFINED
float shuntVoltage_mV = 0.0;
float loadVoltage_V = 0.0;
float busVoltage_V = 0.0;
float current_mA = 0.0;
float power_mW = 0.0; 
bool ina219_overflow = false;
#endif

#ifndef INIT_INA219_DEFINED
#define INIT_INA219_DEFINED
void InitINA219(){
  // if(!ina219.init()){
  //   Serial.println("INA219 not connected!");
  // }
  ina219.init();
  ina219.setADCMode(BIT_MODE_9);
  ina219.setPGain(PG_320);
  ina219.setBusRange(BRNG_16);
  ina219.setShuntSizeInOhms(0.01); // used in INA219.
}
#endif

#ifndef INA_DATA_UPDATE_DEFINED
#define INA_DATA_UPDATE_DEFINED
void InaDataUpdate(){
  shuntVoltage_mV = ina219.getShuntVoltage_mV();
  busVoltage_V = ina219.getBusVoltage_V();
  current_mA = ina219.getCurrent_mA();
  power_mW = ina219.getBusPower();
  loadVoltage_V  = busVoltage_V + (shuntVoltage_mV/1000);
  ina219_overflow = ina219.getOverflow();
}
#endif

// <<<<<<<<<<=========SSD1306: 0x3C===========>>>>>>>>>>
#include <Adafruit_SSD1306.h>
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32

#ifndef DISPLAY_DEFINED
#define DISPLAY_DEFINED
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
#endif

#ifndef DISPLAY_VARIABLES_DEFINED
#define DISPLAY_VARIABLES_DEFINED
int CURRENT_PAGE = 1;
int PAGE_NUM = 2;
int PAGE_FLASH = 3000;
unsigned long LAST_FLASH;
#endif

#ifndef INIT_SCREEN_DEFINED
#define INIT_SCREEN_DEFINED
void InitScreen(){
  // if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
  //   Serial.println(F("SSD1306 allocation failed"));
  // }
  display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS);
  display.clearDisplay();
  display.setTextSize(3);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);
  display.println(F("WAVEGO"));
  display.setTextSize(1);
  display.println(F("ICM20948 calibrating..."));
  display.display();

  LAST_FLASH = millis();
}
#endif

#ifndef XYZ_SCREEN_UPDATE_DEFINED
#define XYZ_SCREEN_UPDATE_DEFINED
void xyzScreenUpdate(float xInput, float yInput, float zInput){
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);

  display.print(int(map(xInput, -17000, 17000, 0, 255)));display.print(F("-"));
  display.print(int(map(yInput, -17000, 17000, 0, 255)));display.print(F("-"));
  display.println(int(map(zInput, -17000, 17000, 0, 255)));

  display.print(F("LoadVoltage:"));display.println(loadVoltage_V);
  display.print(F("Current[mA]:"));display.println(current_mA);
  display.print(F("power[mW]:"));display.println(power_mW);

  display.display();
}
#endif

// Updata all data and flash the screen.
#ifndef ALL_DATA_UPDATE_DEFINED
#define ALL_DATA_UPDATE_DEFINED
void allDataUpdate(){
  if(millis() - LAST_FLASH > PAGE_FLASH && !debugMode){
    CURRENT_PAGE += 1;
    if(CURRENT_PAGE > PAGE_NUM){
      CURRENT_PAGE = 1;
    }
    LAST_FLASH = millis();

    getWifiStatus();
    InaDataUpdate();

    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0,0);

    if(CURRENT_PAGE == 1){
      // 显示网络模式和IP
      if(WIFI_MODE == 1){
        display.print(F("AP:"));
        display.println(IP_ADDRESS_STR);
      }
      else if(WIFI_MODE == 2){
        display.print(F("STA:"));
        display.println(IP_ADDRESS_STR);
      }
      else{
        display.println(F("WIFI:OFF"));
      }
      
      // 显示Xbox状态和电池电压
      display.print(F("Xbox:"));
      display.print(XBOX_CONNECTED ? F("OK ") : F("NO "));
      
      display.print(F("BAT:"));
      display.print(loadVoltage_V,1);
      display.println(F("V"));

      // 显示控制状态
      display.print(F("FB:"));
      display.print(moveFB);
      display.print(F(" LR:"));
      display.print(moveLR);
      display.print(F(" D:"));
      display.print(debugMode);
      display.print(F(" F:"));
      display.println(funcMode);
      
      // 显示步态和速度
      display.print(F("GAIT:"));
      display.print(GAIT_TYPE);
      display.print(F(" SPD:"));
      display.println(STEP_ITERATE);
    }
    else if(CURRENT_PAGE == 2){
      // 在第二页显示Xbox控制器详细信息
      display.println(F("XBOX CONTROLLER"));
      if(XBOX_CONNECTED){
        display.print(F("L("));
        display.print(xboxController.xboxNotif.joyLHori/1000);
        display.print(F(","));
        display.print(xboxController.xboxNotif.joyLVert/1000);
        display.println(F(")"));
        
        display.print(F("R("));
        display.print(xboxController.xboxNotif.joyRHori/1000);
        display.print(F(","));
        display.print(xboxController.xboxNotif.joyRVert/1000);
        display.println(F(")"));
        
        display.print(F("Buttons: "));
        if(xboxController.xboxNotif.btnA) display.print(F("A"));
        if(xboxController.xboxNotif.btnB) display.print(F("B"));
        if(xboxController.xboxNotif.btnY) display.print(F("Y"));
        if(xboxController.xboxNotif.btnX) display.print(F("X"));
        if(xboxController.xboxNotif.btnLB) display.print(F("LB"));
        if(xboxController.xboxNotif.btnRB) display.print(F("RB"));
      } else {
        display.println(F("Not connected"));
        display.println(F("MAC:"));
        display.println(F("68:6c:e6:37:be:26"));
      }
    }
    
    display.display();
  }
  else if(millis() < LAST_FLASH && !debugMode){
    LAST_FLASH = millis();
  }
  else if(debugMode){
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0,0);
    
    // 简化调试模式显示
    for(int i=0; i<16; i+=4) {
      display.print(i);display.print(':');display.print(CurrentPWM[i]);
      display.print(' ');
      display.print(i+1);display.print(':');display.print(CurrentPWM[i+1]);
      display.print(' ');
      display.print(i+2);display.print(':');display.print(CurrentPWM[i+2]);
      display.print(' ');
      display.print(i+3);display.print(':');display.println(CurrentPWM[i+3]);
    }
    
    display.display();
    delay(600);
  }
}
#endif

// <<<<<<<<<<========BUZZER==========>>>>>>>>>>
#ifndef INIT_BUZZER_DEFINED
#define INIT_BUZZER_DEFINED
void InitBuzzer(){
  pinMode(BUZZER, OUTPUT);
  digitalWrite(BUZZER, HIGH);
}
#endif

// <<<<<<<<=========WS2812. (RGB LED)========>>>>>>>>>>
#include <Adafruit_NeoPixel.h>
#define NUMPIXELS   6
#define BRIGHTNESS  255

#ifndef MATRIX_DEFINED
#define MATRIX_DEFINED
Adafruit_NeoPixel matrix = Adafruit_NeoPixel(NUMPIXELS, RGB_LED, NEO_GRB + NEO_KHZ800);
#endif

#ifndef INIT_RGB_DEFINED
#define INIT_RGB_DEFINED
void InitRGB(){
  matrix.setBrightness(BRIGHTNESS);
  matrix.begin();
  matrix.show();
}
#endif

#ifndef COLOR_WIPE_DEFINED
#define COLOR_WIPE_DEFINED
void colorWipe(uint32_t c, uint8_t wait) {
  for(uint16_t i=0; i<matrix.numPixels(); i++) {
    matrix.setPixelColor(i, c);
    matrix.show();
    delay(wait);
  }
}
#endif

#ifndef SET_SINGLE_LED_DEFINED
#define SET_SINGLE_LED_DEFINED
void setSingleLED(uint16_t LEDnum, uint32_t c){
  matrix.setPixelColor(LEDnum, c);
  matrix.show();
}
#endif

#endif // INIT_CONFIG_H