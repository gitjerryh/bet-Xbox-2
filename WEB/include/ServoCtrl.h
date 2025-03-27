#ifndef SERVO_CTRL_H
#define SERVO_CTRL_H

#include <Adafruit_PWMServoDriver.h>
#include <math.h>
// #include "InitConfig.h" // 移除这行，因为main.cpp已经包含了InitConfig.h

// 添加必要的外部声明
extern float ACC_X;
extern float ACC_Y;
extern float ACC_Z;
extern ICM20948_WE myIMU;
extern int funcMode;
extern int debugMode;
extern Adafruit_NeoPixel matrix;
extern void setSingleLED(uint16_t LEDnum, uint32_t c);
#ifndef WIRE_DEBUG
#define WIRE_DEBUG 12
#endif

// 修改此处，使用全局Wire实例而不是创建新的IIC实例
// TwoWire IIC = TwoWire(0);
// Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(0x40, IIC);
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(0x40, Wire);

#define SERVOMIN  263 // This is the 'minimum' pulse length count (out of 4096)
#define SERVOMAX  463 // This is the 'maximum' pulse length count (out of 4096)
#define SERVO_FREQ 50 // Analog servos run at ~50 Hz updates
#define SERVO_RANGE 90
#define M_PI 3.1415926

// 0  Forward 6
// 1  -1|^|3- 7
//<2> --|^|-- <8>
//      |||
//<3> --|^|-- <9>
// 4  -2|^|4- 10
// 5  --|^|-- 11

//   [S-W]---L-W---O
//                 |
//                 |
//                 |
//                 |
//                 |
//               Ground
double Linkage_W = 19.15;   // The distance between wiggle servo and the plane of the leg linkages.

//     <<<[S-A][S-B]<<<
//         /     |
//       L-A    L-A
//       /       |
//      O        O
//      |       /
//     L-B    L-C
//      |     /
//      |    /
//      |   /
//      |  /
//      | /
//      O
//     /  
//   L-D
//   /
//  <---90°
//      .L-E
//           .
// ------------------------
double Linkage_S = 12.2;    // The distance between two servos.
double Linkage_A = 40.0;    // The linkage that connected with the servo.
double Linkage_B = 40.0;    // The linkage that limit the direction.
double Linkage_C = 39.8153; // The upper part of the leg.
double Linkage_D = 31.7750; // The lower part of the leg.
double Linkage_E = 30.8076; // The foot.

double WALK_HEIGHT_MAX  = 110;
double WALK_HEIGHT_MIN  = 75;
double WALK_HEIGHT      = 95;
double WALK_LIFT        = 9; // WALK_HEIGHT + WALK_LIFT <= WALK_HEIGHT_MAX.
double WALK_RANGE       = 40;
double WALK_ACC         = 5;
double WALK_EXTENDED_X  = 16;
double WALK_EXTENDED_Z  = 25;
double WALK_SIDE_MAX    = 30;
double WALK_MASS_ADJUST = 21;
double STAND_HEIGHT     = 95;

uint8_t WALK_STATUS     = 0;
float WALK_CYCLE_GLOBAL = 0;    // 0-1.
float WALK_LIFT_PROP    = 0.25; // WALK_LIFT_TIME<1.

float BALANCE_PITCHU_BUFFER;
float BALANCE_ROLL_BUFFER;
float BALANCE_PITCHU_BASE = 0;
float BALANCE_ROLL_BASE   = 0;
float BALANCE_P = 0.00018;

float GLOBAL_STEP  = 0;
int   STEP_DELAY   = 4;
// int   STEP_DELAY   = 2;
float STEP_ITERATE = 0.04;
// float STEP_ITERATE = 0.01;

int SERVO_MOVE_EVERY = 0;
int MAX_TEST = 125;

#define LEG_A_FORE 8
#define LEG_A_BACK 9
#define LEG_A_WAVE 10

#define LEG_B_WAVE 13
#define LEG_B_FORE 14
#define LEG_B_BACK 15

#define LEG_C_FORE 7
#define LEG_C_BACK 6
#define LEG_C_WAVE 5

#define LEG_D_WAVE 2
#define LEG_D_FORE 1
#define LEG_D_BACK 0


extern int ServoMiddlePWM[16] = {MiddlePosition, MiddlePosition, MiddlePosition, MiddlePosition,
                                 MiddlePosition, MiddlePosition, MiddlePosition, MiddlePosition,
                                 MiddlePosition, MiddlePosition, MiddlePosition, MiddlePosition,
                                 MiddlePosition, MiddlePosition, MiddlePosition, MiddlePosition};

int legPosBuffer[12] = { WALK_EXTENDED_X,  STAND_HEIGHT, WALK_EXTENDED_Z,
                        -WALK_EXTENDED_X,  STAND_HEIGHT, WALK_EXTENDED_Z,
                         WALK_EXTENDED_X,  STAND_HEIGHT, WALK_EXTENDED_Z,
                        -WALK_EXTENDED_X,  STAND_HEIGHT, WALK_EXTENDED_Z};

int GoalPWM[16] = {MiddlePosition, MiddlePosition, MiddlePosition, MiddlePosition,
                   MiddlePosition, MiddlePosition, MiddlePosition, MiddlePosition,
                   MiddlePosition, MiddlePosition, MiddlePosition, MiddlePosition,
                   MiddlePosition, MiddlePosition, MiddlePosition, MiddlePosition};

int LastPWM[16] = {MiddlePosition, MiddlePosition, MiddlePosition, MiddlePosition,
                   MiddlePosition, MiddlePosition, MiddlePosition, MiddlePosition,
                   MiddlePosition, MiddlePosition, MiddlePosition, MiddlePosition,
                   MiddlePosition, MiddlePosition, MiddlePosition, MiddlePosition};

int ServoDirection[16] = {-1,  1,  1,  1,
                           1, -1, -1,  1,
                          -1,  1,  1,  1,
                           1, -1, -1,  1};

double linkageBuffer[32] = {0, 0, 0, 0, 0, 0, 0, 0,
                            0, 0, 0, 0, 0, 0, 0, 0,
                            0, 0, 0, 0, 0, 0, 0, 0,
                            0, 0, 0, 0, 0, 0, 0, 0};


// pre render.
float LAxLA = Linkage_A*Linkage_A;
float LBxLB = Linkage_B*Linkage_B;
float LWxLW = Linkage_W*Linkage_W;
float LExLE = Linkage_E*Linkage_E;
float LAxLA_LBxLB = LAxLA - LBxLB;
float LBxLB_LAxLA = LBxLB - LAxLA;
float L_CD = (Linkage_C+Linkage_D)*(Linkage_C+Linkage_D);
float LAx2  = 2 * Linkage_A;
float LBx2  = 2 * Linkage_B;
float E_PI  = 180 / M_PI;
float LSs2  = Linkage_S/2;
float aLCDE = atan((Linkage_C + Linkage_D)/Linkage_E);
float sLEDC = sqrt(Linkage_E*Linkage_E + (Linkage_D+Linkage_C)*(Linkage_D+Linkage_C));
float O_WLP = 1 - WALK_LIFT_PROP;
float WALK_ACCx2 = WALK_ACC*2;
float WALK_H_L = WALK_HEIGHT - WALK_LIFT;


void ServoSetup(){
  // 移除这里的IIC.begin，因为我们已经在main.cpp中初始化了Wire
  // IIC.begin(S_SDA, S_SCL, 26000000);
  pwm.begin();
  pwm.setOscillatorFrequency(26000000);
  pwm.setPWMFreq(SERVO_FREQ);  // Analog servos run at ~50 Hz updates
  // 移除这里的Wire.setClock，因为我们已经在main.cpp中设置了
  // Wire.setClock(100000);
  Serial.println("16 channel PWM Setup!");
  delay(10);
}


extern void initPosAll(){
  for(int i = 0; i < 16; i++){
    pwm.setPWM(i, 0, MiddlePosition);
    CurrentPWM[i] = MiddlePosition;
    Serial.print(CurrentPWM[i]);
    Serial.print(" ");
    delay(SERVO_MOVE_EVERY);
  }
  Serial.println(" ");
}


extern void middlePosAll(){
  for(int i = 0; i < 16; i++){
    pwm.setPWM(i, 0, ServoMiddlePWM[i]);
    CurrentPWM[i] = ServoMiddlePWM[i];
    Serial.print(CurrentPWM[i]);
    Serial.print(" ");
    delay(SERVO_MOVE_EVERY);
  }
  Serial.println(" ");
}


extern void servoDebug(byte servoID, int offset){
  CurrentPWM[servoID] += offset;
  pwm.setPWM(servoID, 0, CurrentPWM[servoID]);  
}


void GoalPosAll(){
  for(int i = 0; i < 16; i++){
    pwm.setPWM(i, 0, GoalPWM[i]);
  }
}


void goalPWMSet(uint8_t servoNum, double angleInput){
  int pwmGet;
  if (angleInput == 0){pwmGet = 0;}
  else{pwmGet = round((SERVOMAX - SERVOMIN) * angleInput / SERVO_RANGE);}
  pwmGet = pwmGet * ServoDirection[servoNum] + ServoMiddlePWM[servoNum];
  GoalPWM[servoNum] = pwmGet;
}


// Simple Linkage IK:
// input the position of the end and return angle.
//   O----O
//  /
// O
// ---------------------------------------------------
// |       /beta           /delta                    |
//        O----LB---------X------                    |
// |     /       omega.   |       \LB                |
//      LA        .                < ----------------|
// |alpha     .          bIn         \LB -EP  <delta |
//    /psi.                           \LB -EP        |
// | /.   lambda          |                          |
// O- - - - - aIn - - - - X -                        |
// ---------------------------------------------------
// alpha, beta > 0 ; delta <= 0 ; aIn, bIn > 0
// simpleLinkageIK(Linkage_A, Linkage_B, linkageBuffer[yPosBuffer], (linkageBuffer[xPosBuffer]-Linkage_S/2), betaOut, betaB, betaC);
void simpleLinkageIK(double LA, double LB, double aIn, double bIn, uint8_t outputAlpha, uint8_t outputBeta, uint8_t outputDelta){
  double psi;
  double alpha;
  double omega;
  double beta;
  double L2C;
  double LC;
  double lambda;
  double delta;
  if(bIn == 0){
    psi   = acos((LAxLA_LBxLB + aIn * aIn)/(LAx2 * aIn)) * E_PI;
    alpha = 90 - psi;
    omega = acos((aIn * aIn + LBxLB_LAxLA)/(LBx2 * aIn)) * E_PI;
    beta  = psi + omega;
  }
  else{
    L2C = aIn * aIn + bIn * bIn;
    LC  = sqrt(L2C);
    lambda = atan(bIn/aIn) * 180 / M_PI;
    psi    = acos((LAxLA_LBxLB + L2C)/(2 * LA * LC)) * E_PI;
    alpha  = 90 - lambda - psi;
    omega  = acos((LBxLB_LAxLA + L2C)/(2 * LC * LB)) * E_PI;
    beta   = psi + omega;
  }
  delta = 90 - alpha - beta;
  linkageBuffer[outputAlpha] = alpha;
  linkageBuffer[outputBeta]  = beta;
  linkageBuffer[outputDelta] = delta;
}

// ||| ||| ||| ||| ||| |||
// void simpleLinkageIK(double LA, double LB, double aIn, double bIn, uint8_t outputAlpha, uint8_t outputBeta, uint8_t outputDelta){
//   double psi;
//   double alpha;
//   double omega;
//   double beta;
//   double L2C;
//   double LC;
//   double lambda;
//   double delta;
//   if(bIn == 0){
//     psi   = acos((LA * LA + aIn * aIn - LB * LB)/(2 * LA * aIn)) * 180 / M_PI;
//     alpha = 90 - psi;
//     omega = acos((aIn * aIn + LB * LB - LA * LA)/(2 * aIn * LB)) * 180 / M_PI;
//     beta  = psi + omega;
//   }
//   else{
//     L2C = aIn * aIn + bIn * bIn;
//     LC  = sqrt(L2C);
//     lambda = atan(bIn/aIn) * 180 / M_PI;
//     psi    = acos((LA * LA + L2C - LB * LB)/(2 * LA * LC)) * 180 / M_PI;
//     alpha = 90 - lambda - psi;
//     omega = acos((LB * LB + L2C - LA * LA)/(2 * LC * LB)) * 180 / M_PI;
//     beta  = psi + omega;
//   }
//   delta = 90 - alpha - beta;
//   linkageBuffer[outputAlpha] = alpha;
//   linkageBuffer[outputBeta]  = beta;
//   linkageBuffer[outputDelta] = delta;
// }


// Wiggle Plane IK:
// input the position of the end and return angle.
// O-----X
//       |
//       |
//       O
// ------------------------------
//     X                        |
//    /    .                    
//  LA         .                |
//  /alpha         .LB         
// O- - - - - - - - - -.- - - -X|
//                         .  bIn
// ------------aIn-------------X|
// ------------------------------
// alpha, aIn, bIn > 0
// wigglePlaneIK(Linkage_W, zPos, yPos, wiggleAlpha, wiggleLen);
void wigglePlaneIK(double LA, double aIn, double bIn, uint8_t outputAlpha, uint8_t outputLen){
  double LB;
  double L2C;
  double LC;
  double alpha;
  double beta;
  double lambda;
  double psi;
  if(bIn > 0){
    L2C = aIn * aIn + bIn * bIn;
    LC = sqrt(L2C);
    lambda = atan(aIn/bIn) * E_PI;
    psi = acos(LA/LC) * E_PI;
    LB = sqrt(L2C - LWxLW);
    alpha = psi + lambda - 90;
  }
  else if(bIn == 0){
    alpha = asin(LA/aIn) * E_PI;
    L2C = aIn * aIn + bIn * bIn;
    LB = sqrt(L2C);
  }
  else if(bIn < 0){
    bIn = -bIn;
    L2C = aIn * aIn + bIn * bIn;
    LC = sqrt(L2C);
    lambda = atan(aIn/bIn) * E_PI;
    psi = acos(LA/LC) * E_PI;
    LB = sqrt(L2C - LWxLW);
    alpha = 90 - lambda + psi;
  }
  linkageBuffer[outputAlpha] = alpha;
  linkageBuffer[outputLen]  = LB;
}

// ||| ||| ||| ||| ||| |||
// void wigglePlaneIK(double LA, double aIn, double bIn, uint8_t outputAlpha, uint8_t outputLen){
//   double LB;
//   double L2C;
//   double LC;
//   double alpha;
//   double beta;
//   double lambda;
//   double psi;
//   if(bIn > 0){
//     L2C = aIn * aIn + bIn * bIn;
//     LC = sqrt(L2C);
//     lambda = atan(aIn/bIn) * 180 / M_PI;
//     psi = acos(LA/LC) * 180 / M_PI;
//     LB = sqrt(L2C - LA * LA);
//     alpha = psi + lambda - 90;
//   }
//   else if(bIn == 0){
//     alpha = asin(LA/aIn) * 180 / M_PI;
//     L2C = aIn * aIn + bIn * bIn;
//     LB = sqrt(L2C);
//   }
//   else if(bIn < 0){
//     bIn = -bIn;
//     L2C = aIn * aIn + bIn * bIn;
//     LC = sqrt(L2C);
//     lambda = atan(aIn/bIn) * 180 / M_PI;
//     psi = acos(LA/LC) * 180 / M_PI;
//     LB = sqrt(L2C - LA * LA);
//     alpha = 90 - lambda + psi;
//   }
//   linkageBuffer[outputAlpha] = alpha;
//   linkageBuffer[outputLen]  = LB;
// }


// Ctrl single leg plane IK:
// input the args of the leg, return angle and position.
//  SE--------LS(O)-----SE
//             |        |  \.
//                      |   LA
//             |        |beta\.
//       \.                   \.
//        \.   |               O
//         \.              .
//          LB |       LC
//           \.    .
//             O
//         LD
//     .       |
// F
//  \.         |
//   LE       yIn
//    \.       |
//     \.--xIn-X  
// beta > 0 ; xIn, yIn > 0
//   singleLegPlaneIK(Linkage_S, Linkage_A, Linkage_C, Linkage_D, Linkage_E, xPos, linkageBuffer[wiggleLen], alphaOut, xPosBuffer, yPosBuffer);
void singleLegPlaneIK(double LS, double LA, double LC, double LD, double LE, double xIn, double yIn, uint8_t outputBeta, uint8_t outputX, uint8_t outputY){
  double bufferS = sqrt((xIn + LSs2)*(xIn + LSs2) + yIn*yIn);
  double lambda = acos(((xIn + LSs2)*(xIn + LSs2) + yIn*yIn + LAxLA - L_CD - LExLE)/(2*bufferS*LA));
  double delta = atan((xIn + LSs2)/yIn);
  double beta = lambda - delta;
  double betaAngle = beta * E_PI;

  double theta = aLCDE;
  double omega = asin((yIn - cos(beta)*LA)/sLEDC);
  double nu = M_PI - theta - omega;
  double dFX = cos(nu)*LE;
  double dFY = sin(nu)*LE;

  double mu = M_PI/2 - nu;
  double dEX = cos(mu)*LD;
  double dEY = sin(mu)*LD;

  double positionX = xIn + dFX - dEX;
  double positionY = yIn - dFY - dEY;
  
  linkageBuffer[outputBeta] = betaAngle;
  linkageBuffer[outputX]  = positionX;
  linkageBuffer[outputY]  = positionY;
}

// ||| ||| ||| ||| ||| |||
// void singleLegPlaneIK(double LS, double LA, double LC, double LD, double LE, double xIn, double yIn, uint8_t outputBeta, uint8_t outputX, uint8_t outputY){
//   double bufferS = sqrt((xIn + LS/2)*(xIn + LS/2) + yIn*yIn);
//   double lambda = acos(((xIn + LS/2)*(xIn + LS/2) + yIn*yIn + LA*LA - (LC+LD)*(LC+LD) - LE*LE)/(2*bufferS*LA));
//   double delta = atan((xIn + LS/2)/yIn);
//   double beta = lambda - delta;
//   double betaAngle = beta * 180 / M_PI;

//   double theta = atan((LC+LD)/LE);
//   double omega = asin((yIn - cos(beta)*LA)/sqrt(LE*LE + (LD+LC)*(LD+LC)));
//   double nu = M_PI - theta - omega;
//   double dFX = cos(nu)*LE;
//   double dFY = sin(nu)*LE;

//   double positionX = xIn + dFX - dEX;
//   double positionY = yIn - dFY - dEY;
  
//   linkageBuffer[outputBeta] = betaAngle;
//   linkageBuffer[outputX]  = positionX;
//   linkageBuffer[outputY]  = positionY;
// }


// Ctrl a single leg of WAVEGO, it moves in a plane.
// input (x,y) position and return angle alpha and angle beta.
//     O  X  O
//    /         .
//   /    |        O
//  O     y     .
//   \.   |  .
//    \.  .
//     O  |
//  .
//   \.   |
//    \-x-X
// ------------------
// x, y > 0
void singleLegCtrl(uint8_t LegNum, double xPos, double yPos, double zPos){
  uint8_t alphaOut;
  uint8_t xPosBuffer;
  uint8_t yPosBuffer;
  uint8_t betaOut;
  uint8_t betaB;
  uint8_t betaC;
  uint8_t NumF;
  uint8_t NumB;
  uint8_t wiggleAlpha;
  uint8_t wiggleLen;
  uint8_t NumW;
  if(LegNum == 1){
    NumF = LEG_A_FORE;
    NumB = LEG_A_BACK;
    NumW = LEG_A_WAVE;
    alphaOut   = 0;
    xPosBuffer = 1;
    yPosBuffer = 2;
    betaOut = 3;
    betaB   = 4;
    betaC   = 5;
    wiggleAlpha = 6;
    wiggleLen   = 7;
  }
  else if(LegNum == 2){
    NumF = LEG_B_FORE;
    NumB = LEG_B_BACK;
    NumW = LEG_B_WAVE;
    alphaOut   = 8;
    xPosBuffer = 9;
    yPosBuffer = 10;
    betaOut = 11;
    betaB   = 12;
    betaC   = 13;
    wiggleAlpha = 14;
    wiggleLen   = 15;
  }
  else if(LegNum == 3){
    NumF = LEG_C_FORE;
    NumB = LEG_C_BACK;
    NumW = LEG_C_WAVE;
    alphaOut   = 16;
    xPosBuffer = 17;
    yPosBuffer = 18;
    betaOut = 19;
    betaB   = 20;
    betaC   = 21;
    wiggleAlpha = 22;
    wiggleLen   = 23;
  }
  else if(LegNum == 4){
    NumF = LEG_D_FORE;
    NumB = LEG_D_BACK;
    NumW = LEG_D_WAVE;
    alphaOut   = 24;
    xPosBuffer = 25;
    yPosBuffer = 26;
    betaOut = 27;
    betaB   = 28;
    betaC   = 29;
    wiggleAlpha = 30;
    wiggleLen   = 31;
  }

  wigglePlaneIK(Linkage_W, zPos, yPos, wiggleAlpha, wiggleLen);
  singleLegPlaneIK(Linkage_S, Linkage_A, Linkage_C, Linkage_D, Linkage_E, xPos, linkageBuffer[wiggleLen], alphaOut, xPosBuffer, yPosBuffer);
  simpleLinkageIK(Linkage_A, Linkage_B, linkageBuffer[yPosBuffer], (linkageBuffer[xPosBuffer]-Linkage_S/2), betaOut, betaB, betaC);

  goalPWMSet(NumW, linkageBuffer[wiggleAlpha]);
  goalPWMSet(NumF, (90 - linkageBuffer[betaOut]));
  goalPWMSet(NumB, linkageBuffer[alphaOut]);
}


void standUp(double cmdInput){
  singleLegCtrl(1, WALK_EXTENDED_X, cmdInput, WALK_EXTENDED_Z);
  singleLegCtrl(2, -WALK_EXTENDED_X, cmdInput, WALK_EXTENDED_Z);
  singleLegCtrl(3, WALK_EXTENDED_X, cmdInput, WALK_EXTENDED_Z);
  singleLegCtrl(4, -WALK_EXTENDED_X, cmdInput, WALK_EXTENDED_Z);
}


// Ctrl the gait of a single leg with the variable cycleInput change between 0-1.
// when the directionInput > 0, the front point in the gait cycle move away from the middle line of the robot.
// use the extendedX and extendedZ to adjust the position of the middle point in a wiggle cycle.
// statusInput used to reduce the WALK_RANGE.
void singleGaitCtrl(uint8_t LegNum, uint8_t statusInput, float cycleInput, float directionInput, double extendedX, double extendedZ){
  double rDist;
  double xGait;
  double yGait;
  double zGait;
  double rDiection = directionInput * M_PI / 180;
  
  // 添加静态缓冲区以平滑腿部运动
  static double lastXGait[5] = {0, 0, 0, 0, 0}; // 每条腿一个缓冲区，索引0不使用
  static double lastYGait[5] = {0, 0, 0, 0, 0};
  static double lastZGait[5] = {0, 0, 0, 0, 0};
  
  // 平滑系数 - 更高的值表示更多的平滑
  const float SMOOTHING_FACTOR = 0.4;

  // 计算Y位置（高度）- 使用正弦曲线替代原来的分段线性，提供更平滑的过渡
  if(cycleInput < (1 - WALK_LIFT_PROP)){
    // 下降阶段 - 使用贝塞尔曲线获得更平滑的过渡
    double normalizedCycle = cycleInput / (1 - WALK_LIFT_PROP);
    
    // 贝塞尔曲线参数 - 调整控制点可以改变曲线形状
    double p0 = WALK_HEIGHT - WALK_LIFT;
    double p1 = WALK_HEIGHT - WALK_LIFT * 0.7;
    double p2 = WALK_HEIGHT - WALK_LIFT * 0.3;
    double p3 = WALK_HEIGHT;
    
    if (normalizedCycle < 0.5) {
      // 前半部分贝塞尔曲线
      double t = normalizedCycle * 2; // 归一化为0-1区间
      yGait = (1-t)*(1-t)*(1-t)*p0 + 3*(1-t)*(1-t)*t*p1 + 3*(1-t)*t*t*p2 + t*t*t*p3;
    } else {
      // 后半部分贝塞尔曲线
      double t = (normalizedCycle - 0.5) * 2; // 归一化为0-1区间
      yGait = (1-t)*(1-t)*(1-t)*p3 + 3*(1-t)*(1-t)*t*p2 + 3*(1-t)*t*t*p1 + t*t*t*p0;
    }
    
    // 计算水平位移，使用更平滑的正弦曲线
    double phaseShift = M_PI * (1 - normalizedCycle);
    rDist = cos(phaseShift) * (WALK_RANGE * statusInput * 0.5);
  }
  else if(cycleInput >= (1 - WALK_LIFT_PROP)){
    // 抬腿阶段 - 保持高度不变，使用正弦曲线进行平移
    yGait = WALK_HEIGHT - WALK_LIFT;
    
    // 归一化为0-1区间
    double liftPhase = (cycleInput - (1 - WALK_LIFT_PROP)) / WALK_LIFT_PROP;
    
    // 使用正弦曲线，在π到2π之间映射，以获得平滑转换
    rDist = sin(liftPhase * M_PI + M_PI) * (WALK_RANGE * statusInput * 0.5);
  }

  // 计算XZ平面上的位置
  xGait = cos(rDiection) * rDist;
  zGait = sin(rDiection) * rDist;
  
  // 应用平滑处理 - 当前位置和上一个位置的加权平均
  if (lastXGait[LegNum] != 0 || lastYGait[LegNum] != 0 || lastZGait[LegNum] != 0) {
    xGait = (1 - SMOOTHING_FACTOR) * xGait + SMOOTHING_FACTOR * lastXGait[LegNum];
    yGait = (1 - SMOOTHING_FACTOR) * yGait + SMOOTHING_FACTOR * lastYGait[LegNum];
    zGait = (1 - SMOOTHING_FACTOR) * zGait + SMOOTHING_FACTOR * lastZGait[LegNum];
  }
  
  // 保存当前位置用于下次平滑
  lastXGait[LegNum] = xGait;
  lastYGait[LegNum] = yGait;
  lastZGait[LegNum] = zGait;
  
  // 控制单腿位置
  singleLegCtrl(LegNum, (xGait + extendedX), yGait, (zGait + extendedZ));
}


// a simple gait to ctrl the robot.
// GlobalInput changes between 0-1.
// use directionAngle to ctrl the direction.
// turnCmd-> -1:left 1:right
void simpleGait(float GlobalInput, float directionAngle, int turnCmd){
  float Group_A;
  float Group_B;

  Group_A = GlobalInput;
  Group_B = GlobalInput+0.5;
  if(Group_B>1){Group_B--;}

  if(!turnCmd){
    singleGaitCtrl(1, 1, Group_A, directionAngle,  WALK_EXTENDED_X, WALK_EXTENDED_Z);
    singleGaitCtrl(4, 1, Group_A, -directionAngle, -WALK_EXTENDED_X, WALK_EXTENDED_Z);

    singleGaitCtrl(2, 1, Group_B, directionAngle, -WALK_EXTENDED_X, WALK_EXTENDED_Z);
    singleGaitCtrl(3, 1, Group_B, -directionAngle, WALK_EXTENDED_X, WALK_EXTENDED_Z);
  }
  else if(turnCmd == -1){
    singleGaitCtrl(1, 1.5, Group_A, 90,  WALK_EXTENDED_X, WALK_EXTENDED_Z);
    singleGaitCtrl(4, 1.5, Group_A, 90, -WALK_EXTENDED_X, WALK_EXTENDED_Z);

    singleGaitCtrl(2, 1.5, Group_B, -90, -WALK_EXTENDED_X, WALK_EXTENDED_Z);
    singleGaitCtrl(3, 1.5, Group_B, -90, WALK_EXTENDED_X, WALK_EXTENDED_Z);
  }
  else if(turnCmd == 1){
    singleGaitCtrl(1, 1.5, Group_A, -90,  WALK_EXTENDED_X, WALK_EXTENDED_Z);
    singleGaitCtrl(4, 1.5, Group_A, -90, -WALK_EXTENDED_X, WALK_EXTENDED_Z);

    singleGaitCtrl(2, 1.5, Group_B, 90, -WALK_EXTENDED_X, WALK_EXTENDED_Z);
    singleGaitCtrl(3, 1.5, Group_B, 90, WALK_EXTENDED_X, WALK_EXTENDED_Z);
  }
}


// Triangular gait generater.
void triangularGait(float GlobalInput, float directionAngle, int turnCmd){
  float StepA;
  float StepB;
  float StepC;
  float StepD;

  float aInput = 0;
  float bInput = 0;
  float adProp;

  StepB = GlobalInput;
  StepC = GlobalInput + 0.25;
  StepD = GlobalInput + 0.5;
  StepA = GlobalInput + 0.75;

  if(StepA>1){StepA--;}
  if(StepB>1){StepB--;}
  if(StepC>1){StepC--;}
  if(StepD>1){StepD--;}

  if(GlobalInput <= 0.25){
    adProp = GlobalInput;
    aInput =  WALK_MASS_ADJUST - (adProp/0.125)*WALK_MASS_ADJUST;
    bInput = -WALK_MASS_ADJUST;
  }
  else if(GlobalInput > 0.25 && GlobalInput <= 0.5){
    adProp = GlobalInput-0.25;
    aInput = -WALK_MASS_ADJUST + (adProp/0.125)*WALK_MASS_ADJUST;
    bInput = -WALK_MASS_ADJUST + (adProp/0.125)*WALK_MASS_ADJUST;
  }
  else if(GlobalInput > 0.5 && GlobalInput <= 0.75){
    adProp = GlobalInput-0.5;
    aInput =  WALK_MASS_ADJUST - (adProp/0.125)*WALK_MASS_ADJUST;
    bInput =  WALK_MASS_ADJUST;
  }
  else if(GlobalInput > 0.75 && GlobalInput <= 1){
    adProp = GlobalInput-0.75;
    aInput = -WALK_MASS_ADJUST + (adProp/0.125)*WALK_MASS_ADJUST;
    bInput =  WALK_MASS_ADJUST - (adProp/0.125)*WALK_MASS_ADJUST;
  }

  if(!turnCmd){
    singleGaitCtrl(1, 1, StepA,  directionAngle,  WALK_EXTENDED_X - aInput, WALK_EXTENDED_Z - bInput);
    singleGaitCtrl(4, 1, StepD, -directionAngle, -WALK_EXTENDED_X - aInput, WALK_EXTENDED_Z + bInput);

    singleGaitCtrl(2, 1, StepB,  directionAngle, -WALK_EXTENDED_X - aInput, WALK_EXTENDED_Z - bInput);
    singleGaitCtrl(3, 1, StepC, -directionAngle,  WALK_EXTENDED_X - aInput, WALK_EXTENDED_Z + bInput);
  }
  else if(turnCmd == -1){
    singleGaitCtrl(1, 1.5, StepA,  90,  WALK_EXTENDED_X - aInput, WALK_EXTENDED_Z - bInput);
    singleGaitCtrl(4, 1.5, StepD,  90, -WALK_EXTENDED_X - aInput, WALK_EXTENDED_Z + bInput);

    singleGaitCtrl(2, 1.5, StepB, -90, -WALK_EXTENDED_X - aInput, WALK_EXTENDED_Z - bInput);
    singleGaitCtrl(3, 1.5, StepC, -90,  WALK_EXTENDED_X - aInput, WALK_EXTENDED_Z + bInput);
  }
  else if(turnCmd == 1){
    singleGaitCtrl(1, 1.5, StepA, -90,  WALK_EXTENDED_X - aInput, WALK_EXTENDED_Z - bInput);
    singleGaitCtrl(4, 1.5, StepD, -90, -WALK_EXTENDED_X - aInput, WALK_EXTENDED_Z + bInput);

    singleGaitCtrl(2, 1.5, StepB,  90, -WALK_EXTENDED_X - aInput, WALK_EXTENDED_Z - bInput);
    singleGaitCtrl(3, 1.5, StepC,  90,  WALK_EXTENDED_X - aInput, WALK_EXTENDED_Z + bInput);
  }
}


// Gait Select
void gaitTypeCtrl(float GlobalStepInput, float directionCmd, int turnCmd){
  if(GAIT_TYPE == 0){
    simpleGait(GlobalStepInput, directionCmd, turnCmd);
  }
  else if(GAIT_TYPE == 1){
    triangularGait(GlobalStepInput, directionCmd, turnCmd);
  }
}


// Stand and adjust mass center.
//    |
//    a
//    |
// -b-M  a,b > 0
void standMassCenter(float aInput, float bInput){
  singleLegCtrl(1, ( WALK_EXTENDED_X - aInput), STAND_HEIGHT, ( WALK_EXTENDED_Z - bInput));
  singleLegCtrl(2, (-WALK_EXTENDED_X - aInput), STAND_HEIGHT, ( WALK_EXTENDED_Z - bInput));

  singleLegCtrl(3, ( WALK_EXTENDED_X - aInput), STAND_HEIGHT, ( WALK_EXTENDED_Z + bInput));
  singleLegCtrl(4, (-WALK_EXTENDED_X - aInput), STAND_HEIGHT, ( WALK_EXTENDED_Z + bInput));
}


// ctrl pitch yaw and roll.
// pitchInput (-, +), if > 0, look up.
// yawInput   (-, +), if > 0, look right.
// rollInput  (-, +), if > 0, lean right.
void pitchYawRoll(float pitchInput, float yawInput, float rollInput){
  legPosBuffer[1]  = STAND_HEIGHT + pitchInput + rollInput;
  legPosBuffer[4]  = STAND_HEIGHT - pitchInput + rollInput;
  legPosBuffer[7]  = STAND_HEIGHT + pitchInput - rollInput;
  legPosBuffer[10] = STAND_HEIGHT - pitchInput - rollInput;

  if(legPosBuffer[1] > WALK_HEIGHT_MAX){legPosBuffer[1] = WALK_HEIGHT_MAX;}
  if(legPosBuffer[4] > WALK_HEIGHT_MAX){legPosBuffer[4] = WALK_HEIGHT_MAX;}
  if(legPosBuffer[7] > WALK_HEIGHT_MAX){legPosBuffer[7] = WALK_HEIGHT_MAX;}
  if(legPosBuffer[10] > WALK_HEIGHT_MAX){legPosBuffer[10] = WALK_HEIGHT_MAX;}
  
  if(legPosBuffer[1] < WALK_HEIGHT_MIN){legPosBuffer[1] = WALK_HEIGHT_MIN;}
  if(legPosBuffer[4] < WALK_HEIGHT_MIN){legPosBuffer[4] = WALK_HEIGHT_MIN;}
  if(legPosBuffer[7] < WALK_HEIGHT_MIN){legPosBuffer[7] = WALK_HEIGHT_MIN;}
  if(legPosBuffer[10] < WALK_HEIGHT_MIN){legPosBuffer[10] = WALK_HEIGHT_MIN;}

  legPosBuffer[2]  = WALK_EXTENDED_Z + yawInput - rollInput;
  legPosBuffer[5]  = WALK_EXTENDED_Z - yawInput - rollInput;
  legPosBuffer[8]  = WALK_EXTENDED_Z - yawInput + rollInput;
  legPosBuffer[11] = WALK_EXTENDED_Z + yawInput + rollInput;

  if(legPosBuffer[2] > WALK_EXTENDED_Z + WALK_SIDE_MAX){legPosBuffer[2] = WALK_EXTENDED_Z + WALK_SIDE_MAX;}
  if(legPosBuffer[5] > WALK_EXTENDED_Z + WALK_SIDE_MAX){legPosBuffer[5] = WALK_EXTENDED_Z + WALK_SIDE_MAX;}
  if(legPosBuffer[8] > WALK_EXTENDED_Z + WALK_SIDE_MAX){legPosBuffer[8] = WALK_EXTENDED_Z + WALK_SIDE_MAX;}
  if(legPosBuffer[11] > WALK_EXTENDED_Z + WALK_SIDE_MAX){legPosBuffer[11] = WALK_EXTENDED_Z + WALK_SIDE_MAX;}

  if(legPosBuffer[2] < WALK_EXTENDED_Z - WALK_SIDE_MAX){legPosBuffer[2] = WALK_EXTENDED_Z - WALK_SIDE_MAX;}
  if(legPosBuffer[5] < WALK_EXTENDED_Z - WALK_SIDE_MAX){legPosBuffer[5] = WALK_EXTENDED_Z - WALK_SIDE_MAX;}
  if(legPosBuffer[8] < WALK_EXTENDED_Z - WALK_SIDE_MAX){legPosBuffer[8] = WALK_EXTENDED_Z - WALK_SIDE_MAX;}
  if(legPosBuffer[11] < WALK_EXTENDED_Z - WALK_SIDE_MAX){legPosBuffer[11] = WALK_EXTENDED_Z - WALK_SIDE_MAX;}

  singleLegCtrl(1,  WALK_EXTENDED_X, legPosBuffer[1] , legPosBuffer[2]);
  singleLegCtrl(2, -WALK_EXTENDED_X, legPosBuffer[4] , legPosBuffer[5]);
  singleLegCtrl(3,  WALK_EXTENDED_X, legPosBuffer[7] , legPosBuffer[8]);
  singleLegCtrl(4, -WALK_EXTENDED_X, legPosBuffer[10], legPosBuffer[11]);
}


void pitchYawRollHeightCtrl(float pitchInput, float yawInput, float rollInput, float heightInput){
  legPosBuffer[1]  = STAND_HEIGHT + pitchInput + rollInput + heightInput;
  legPosBuffer[4]  = STAND_HEIGHT - pitchInput + rollInput + heightInput;
  legPosBuffer[7]  = STAND_HEIGHT + pitchInput - rollInput + heightInput;
  legPosBuffer[10] = STAND_HEIGHT - pitchInput - rollInput + heightInput;

  if(legPosBuffer[1] > WALK_HEIGHT_MAX){legPosBuffer[1] = WALK_HEIGHT_MAX;}
  if(legPosBuffer[4] > WALK_HEIGHT_MAX){legPosBuffer[4] = WALK_HEIGHT_MAX;}
  if(legPosBuffer[7] > WALK_HEIGHT_MAX){legPosBuffer[7] = WALK_HEIGHT_MAX;}
  if(legPosBuffer[10] > WALK_HEIGHT_MAX){legPosBuffer[10] = WALK_HEIGHT_MAX;}
  
  if(legPosBuffer[1] < WALK_HEIGHT_MIN){legPosBuffer[1] = WALK_HEIGHT_MIN;}
  if(legPosBuffer[4] < WALK_HEIGHT_MIN){legPosBuffer[4] = WALK_HEIGHT_MIN;}
  if(legPosBuffer[7] < WALK_HEIGHT_MIN){legPosBuffer[7] = WALK_HEIGHT_MIN;}
  if(legPosBuffer[10] < WALK_HEIGHT_MIN){legPosBuffer[10] = WALK_HEIGHT_MIN;}

  legPosBuffer[2]  = WALK_EXTENDED_Z + yawInput - rollInput;
  legPosBuffer[5]  = WALK_EXTENDED_Z - yawInput - rollInput;
  legPosBuffer[8]  = WALK_EXTENDED_Z - yawInput + rollInput;
  legPosBuffer[11] = WALK_EXTENDED_Z + yawInput + rollInput;

  if(legPosBuffer[2] > WALK_EXTENDED_Z + WALK_SIDE_MAX){legPosBuffer[2] = WALK_EXTENDED_Z + WALK_SIDE_MAX;}
  if(legPosBuffer[5] > WALK_EXTENDED_Z + WALK_SIDE_MAX){legPosBuffer[5] = WALK_EXTENDED_Z + WALK_SIDE_MAX;}
  if(legPosBuffer[8] > WALK_EXTENDED_Z + WALK_SIDE_MAX){legPosBuffer[8] = WALK_EXTENDED_Z + WALK_SIDE_MAX;}
  if(legPosBuffer[11] > WALK_EXTENDED_Z + WALK_SIDE_MAX){legPosBuffer[11] = WALK_EXTENDED_Z + WALK_SIDE_MAX;}

  if(legPosBuffer[2] < WALK_EXTENDED_Z - WALK_SIDE_MAX){legPosBuffer[2] = WALK_EXTENDED_Z - WALK_SIDE_MAX;}
  if(legPosBuffer[5] < WALK_EXTENDED_Z - WALK_SIDE_MAX){legPosBuffer[5] = WALK_EXTENDED_Z - WALK_SIDE_MAX;}
  if(legPosBuffer[8] < WALK_EXTENDED_Z - WALK_SIDE_MAX){legPosBuffer[8] = WALK_EXTENDED_Z - WALK_SIDE_MAX;}
  if(legPosBuffer[11] < WALK_EXTENDED_Z - WALK_SIDE_MAX){legPosBuffer[11] = WALK_EXTENDED_Z - WALK_SIDE_MAX;}

  singleLegCtrl(1,  WALK_EXTENDED_X, legPosBuffer[1] , legPosBuffer[2]);
  singleLegCtrl(2, -WALK_EXTENDED_X, legPosBuffer[4] , legPosBuffer[5]);
  singleLegCtrl(3,  WALK_EXTENDED_X, legPosBuffer[7] , legPosBuffer[8]);
  singleLegCtrl(4, -WALK_EXTENDED_X, legPosBuffer[10], legPosBuffer[11]);
}


// BALANCE
void balancing(){
  if(funcMode == 1){
    // 确保每次调用都更新传感器数据
    accXYZUpdate();
    
    // 添加静态校准偏移量，用于补偿安装误差
    static float pitch_offset = 0;
    static float roll_offset = 0;
    static bool calibrated = false;
    
    // 获取ICM20948的陀螺仪数据
    xyzFloat gyrRaw;
    myIMU.getGyrValues(&gyrRaw);
    float gyro_x = gyrRaw.x; // 绕X轴的角速度
    float gyro_y = gyrRaw.y; // 绕Y轴的角速度
    float gyro_z = gyrRaw.z; // 绕Z轴的角速度
    
    // 首次进入steady模式时校准传感器基准
    if(!calibrated) {
      // 采集35个样本求平均值作为基准偏移量
      float p_sum = 0, r_sum = 0;
      for(int i=0; i<35; i++) {
        accXYZUpdate();
        p_sum += atan2(ACC_X, sqrt(ACC_Y*ACC_Y + ACC_Z*ACC_Z)) * 180.0 / M_PI;
        r_sum += atan2(ACC_Y, sqrt(ACC_X*ACC_X + ACC_Z*ACC_Z)) * 180.0 / M_PI;
        delay(20);
      }
      pitch_offset = p_sum / 35.0;
      roll_offset = r_sum / 35.0;
      calibrated = true;
      
      Serial.print("校准完成 - Pitch偏移: "); 
      Serial.print(pitch_offset);
      Serial.print(" Roll偏移: "); 
      Serial.println(roll_offset);
    }
    
    // 采用互补滤波器融合加速度计和陀螺仪数据
    // 计算加速度计倾角
    float accel_pitch = atan2(ACC_X, sqrt(ACC_Y*ACC_Y + ACC_Z*ACC_Z)) * 180.0 / M_PI - pitch_offset;
    float accel_roll = atan2(ACC_Y, sqrt(ACC_X*ACC_X + ACC_Z*ACC_Z)) * 180.0 / M_PI - roll_offset;
    
    // 平滑处理加速度计读数 - 采用适中的滤波系数
    static float last_accel_pitch = 0;
    static float last_accel_roll = 0;
    
    accel_pitch = 0.75 * accel_pitch + 0.25 * last_accel_pitch;
    accel_roll = 0.75 * accel_roll + 0.25 * last_accel_roll;
    
    last_accel_pitch = accel_pitch;
    last_accel_roll = accel_roll;
    
    // 使用互补滤波器融合陀螺仪数据和加速度计数据
    static float pitch_angle = 0;
    static float roll_angle = 0;
    static unsigned long last_time = 0;
    
    unsigned long current_time = millis();
    float dt = (current_time - last_time) / 1000.0; // 转换为秒
    if(dt <= 0 || dt > 0.1) dt = 0.01; // 防止首次运行、计时器溢出或长时间无调用
    last_time = current_time;
    
    // 增加陀螺仪漂移纠正
    static unsigned long drift_correction_timer = 0;
    const unsigned long DRIFT_CORRECTION_INTERVAL = 15000; // 修改为15秒检查一次漂移，原为5秒
    
    if (current_time - drift_correction_timer > DRIFT_CORRECTION_INTERVAL) {
      // 在静止状态下进行漂移校正
      if (abs(gyro_x) < 0.5 && abs(gyro_y) < 0.5 && abs(gyro_z) < 0.5) {
        // 减小校正幅度，使动作更加平滑（从0.2降到0.05）
        pitch_angle = 0.95 * pitch_angle + 0.05 * accel_pitch;
        roll_angle = 0.95 * roll_angle + 0.05 * accel_roll;
        
        // 使用更平滑的校正，无需打印信息以减少串口负担
        // Serial.println("执行漂移校正");
      }
      drift_correction_timer = current_time;
    }
    
    // 互补滤波 - 改进的alpha值提供更好的长期稳定性
    float alpha = 0.98; // 稍微减少陀螺仪的影响，增加加速度计的权重
    pitch_angle = alpha * (pitch_angle + gyro_x * dt) + (1 - alpha) * accel_pitch;
    roll_angle = alpha * (roll_angle + gyro_y * dt) + (1 - alpha) * accel_roll;
    
    // 限制异常传感器数据
    if(pitch_angle > 30) pitch_angle = 30;
    if(pitch_angle < -30) pitch_angle = -30;
    if(roll_angle > 30) roll_angle = 30;
    if(roll_angle < -30) roll_angle = -30;
    
    // 应用适度的滤波平滑
    static float filtered_pitch = 0;
    static float filtered_roll = 0;
    
    filtered_pitch = filtered_pitch * 0.85 + pitch_angle * 0.15;
    filtered_roll = filtered_roll * 0.85 + roll_angle * 0.15;
    
    // 适中的死区范围
    if(abs(filtered_pitch) < 1.2) filtered_pitch = 0;
    if(abs(filtered_roll) < 1.2) filtered_roll = 0;
    
    // 实现完整的PID控制器
    static float lastPitch = 0;
    static float lastRoll = 0;
    static float integral_pitch = 0;
    static float integral_roll = 0;
    
    // 增加积分项衰减，防止长时间积累
    integral_pitch *= 0.95; 
    integral_roll *= 0.95;
    
    // 积分控制设置
    if(abs(filtered_pitch) < 6) {
      integral_pitch += filtered_pitch * dt * 0.1;
    } else {
      integral_pitch = 0;
    }
    
    if(abs(filtered_roll) < 6) {
      integral_roll += filtered_roll * dt * 0.1;
    } else {
      integral_roll = 0;
    }
    
    // 防止积分饱和
    float integral_limit = 5.0;
    if(integral_pitch > integral_limit) integral_pitch = integral_limit;
    if(integral_pitch < -integral_limit) integral_pitch = -integral_limit;
    if(integral_roll > integral_limit) integral_roll = integral_limit;
    if(integral_roll < -integral_limit) integral_roll = -integral_limit;
    
    // 调整PID参数
    float P_pitch = 0.5;
    float I_pitch = 0.01;
    float D_pitch = 0.15;
    
    float P_roll = 0.5;
    float I_roll = 0.01;
    float D_roll = 0.15;
    
    // 微分项计算
    float derivative_pitch = (filtered_pitch - lastPitch) / dt;
    float derivative_roll = (filtered_roll - lastRoll) / dt;
    
    // 加入微分项的低通滤波以减少高频噪声放大
    static float last_derivative_pitch = 0;
    static float last_derivative_roll = 0;
    
    derivative_pitch = 0.7 * derivative_pitch + 0.3 * last_derivative_pitch;
    derivative_roll = 0.7 * derivative_roll + 0.3 * last_derivative_roll;
    
    last_derivative_pitch = derivative_pitch;
    last_derivative_roll = derivative_roll;
    
    // 映射系数
    float targetPitch = -filtered_pitch * 4.0; 
    float targetRoll = filtered_roll * 4.0;  
    
    // 目标角度限制
    if(targetPitch > 12) targetPitch = 12;  
    if(targetPitch < -12) targetPitch = -12;
    if(targetRoll > 12) targetRoll = 12;
    if(targetRoll < -12) targetRoll = -12;
    
    float outputPitch = P_pitch * targetPitch + I_pitch * integral_pitch + D_pitch * derivative_pitch;
    float outputRoll = P_roll * targetRoll + I_roll * integral_roll + D_roll * derivative_roll;
    
    // 记录当前值供下次使用
    lastPitch = filtered_pitch;
    lastRoll = filtered_roll;
    
    // 平衡的输出平滑处理
    static float lastOutputPitch = 0;
    static float lastOutputRoll = 0;
    
    float maxChange = 1.2;
    if(abs(outputPitch - lastOutputPitch) > maxChange) {
      outputPitch = lastOutputPitch + (outputPitch > lastOutputPitch ? maxChange : -maxChange);
    }
    if(abs(outputRoll - lastOutputRoll) > maxChange) {
      outputRoll = lastOutputRoll + (outputRoll > lastOutputRoll ? maxChange : -maxChange);
    }
    
    // 二次平滑
    outputPitch = 0.7 * outputPitch + 0.3 * lastOutputPitch;
    outputRoll = 0.7 * outputRoll + 0.3 * lastOutputRoll;
    
    // 添加输出限制
    if(outputPitch > 15) outputPitch = 15;
    if(outputPitch < -15) outputPitch = -15;
    if(outputRoll > 15) outputRoll = 15;
    if(outputRoll < -15) outputRoll = -15;
    
    lastOutputPitch = outputPitch;
    lastOutputRoll = outputRoll;
    
    // 添加自动复位逻辑
    static unsigned long auto_reset_timer = 0;
    
    if (current_time - auto_reset_timer > 60000) { // 每60秒检查一次
      if (abs(filtered_pitch) < 3 && abs(outputPitch) < 5) {
        outputPitch *= 0.8;
      }
      if (abs(filtered_roll) < 3 && abs(outputRoll) < 5) {
        outputRoll *= 0.8;
      }
      auto_reset_timer = current_time;
      Serial.println("执行微调复位");
    }
    
    // 应用到机械狗姿态控制
    pitchYawRoll(outputPitch, 0, outputRoll);
    
    // 每次循环输出一次状态信息
    static unsigned long lastPrintTime = 0;
    if(millis() - lastPrintTime >= 200) {
      lastPrintTime = millis();
      Serial.print("平衡 - P: "); Serial.print(filtered_pitch);
      Serial.print(" R: "); Serial.print(filtered_roll);
      Serial.print(" Out P: "); Serial.print(outputPitch);
      Serial.print(" R: "); Serial.println(outputRoll);
    }
  } else {
    // 当退出Steady模式时重置校准状态
    static bool resetCalibration = true;
    if(resetCalibration) {
      Serial.println("退出Steady模式");
      resetCalibration = false;
    }
    
    if(funcMode == 1) {
      resetCalibration = true;
    }
  }
}


// mass center adjust test.
void massCenerAdjustTestLoop(){
  for(float i = 0; i<=20; i = i+0.6){
    standMassCenter(-20+i, i);
    GoalPosAll();
    delay(0);
  }

  for(float i = 0; i<=20; i = i+0.6){
    standMassCenter(i, 20-i);
    GoalPosAll();
    delay(0);
  }

  for(float i = 0; i<=20; i = i+0.6){
    standMassCenter(20-i, -i);
    GoalPosAll();
    delay(0);
  }

  for(float i = 0; i<=20; i = i+0.6){
    standMassCenter(-i, -20+i);
    GoalPosAll();
    delay(0);
  }
}


// pitch yaw roll test.
void pitchYawRollTestLoop(){
  for(int i = -23; i<23; i++){
    pitchYawRoll(0, 0, i);
    GoalPosAll();
    delay(10);
  }
  for(int i = 23; i>-23; i--){
    pitchYawRoll(0, 0, i);
    GoalPosAll();
    delay(10);
  }

  for(int i = -23; i<23; i++){
    pitchYawRoll(i, 0, 0);
    GoalPosAll();
    delay(10);
  }
  for(int i = 23; i>-23; i--){
    pitchYawRoll(i, 0, 0);
    GoalPosAll();
    delay(10);
  }
}


// key frames ctrl.
// 0 <= rateInput <= 1
// use it fuction to ctrl a number change from numStart to numEnd.
float linearCtrl(float numStart, float numEnd, float rateInput){
  float numOut;
  numOut = (numEnd - numStart)*rateInput + numStart;
  return numOut;
}


// linearCtrl() function is a simple example, which shows how besselCtrl works.
float besselCtrl(float numStart, float numEnd, float rateInput){
  float numOut;
  numOut = (numEnd - numStart)*((cos(rateInput*M_PI-M_PI)+1)/2) + numStart;
  return numOut;
}


// Functions.
void functionStayLow(){
  for(float i = 0; i<=1; i+=0.02){
    standUp(besselCtrl(WALK_HEIGHT, WALK_HEIGHT_MIN, i));
    GoalPosAll();
    delay(1);
  }
  delay(300);
  for(float i = 0; i<=1; i+=0.02){
    standUp(besselCtrl(WALK_HEIGHT_MIN, WALK_HEIGHT_MAX, i));
    GoalPosAll();
    delay(1);
  }
  for(float i = 0; i<=1; i+=0.02){
    standUp(besselCtrl(WALK_HEIGHT_MAX, WALK_HEIGHT, i));
    GoalPosAll();
    delay(1);
  }
}


void functionHandshake(){
  for(float i = 0; i<=1; i+=0.02){
    singleLegCtrl(1,  besselCtrl(WALK_EXTENDED_X, 0, i), besselCtrl(WALK_HEIGHT, WALK_HEIGHT_MAX, i), besselCtrl(WALK_EXTENDED_Z, -15, i));
    singleLegCtrl(3,  besselCtrl(WALK_EXTENDED_X, 0, i), besselCtrl(WALK_HEIGHT, WALK_HEIGHT_MAX, i), WALK_EXTENDED_Z);

    singleLegCtrl(2,  -WALK_EXTENDED_X, besselCtrl(WALK_HEIGHT, WALK_HEIGHT_MIN-10, i), besselCtrl(WALK_EXTENDED_Z, 2*WALK_EXTENDED_Z, i));
    singleLegCtrl(4,  -WALK_EXTENDED_X, besselCtrl(WALK_HEIGHT, WALK_HEIGHT_MIN-10, i), besselCtrl(WALK_EXTENDED_Z, 2*WALK_EXTENDED_Z, i));

    GoalPosAll();
    delay(1);
  }


  for(float i = 0; i<=1; i+=0.02){
    singleLegCtrl(3,  besselCtrl(0, WALK_RANGE/2+WALK_EXTENDED_X, i), besselCtrl(WALK_HEIGHT_MAX, WALK_HEIGHT_MIN, i), besselCtrl(WALK_EXTENDED_Z, 0, i));

    GoalPosAll();
    delay(1);
  }

  for(int shakeTimes = 0; shakeTimes < 3; shakeTimes++){
    for(float i = 0; i<=1; i+=0.03){
      singleLegCtrl(3,  WALK_RANGE/2+WALK_EXTENDED_X, besselCtrl(WALK_HEIGHT_MIN, WALK_HEIGHT_MIN+30, i), 0);

      GoalPosAll();
      delay(1);
    }
    for(float i = 0; i<=1; i+=0.03){
      singleLegCtrl(3,  WALK_RANGE/2+WALK_EXTENDED_X, besselCtrl(WALK_HEIGHT_MIN+30, WALK_HEIGHT_MIN, i), 0);

      GoalPosAll();
      delay(1);
    }
  }

  for(float i = 0; i<=1; i+=0.02){
    singleLegCtrl(1,  besselCtrl(0, WALK_EXTENDED_X, i), besselCtrl(WALK_HEIGHT_MAX, WALK_HEIGHT, i), besselCtrl(-15, WALK_EXTENDED_Z, i));
    singleLegCtrl(3,  besselCtrl(WALK_RANGE/2+WALK_EXTENDED_X, WALK_EXTENDED_X, i), besselCtrl(WALK_HEIGHT_MIN, WALK_HEIGHT, i), besselCtrl(0, WALK_EXTENDED_Z, i));

    singleLegCtrl(2,  -WALK_EXTENDED_X, besselCtrl(WALK_HEIGHT_MIN-10, WALK_HEIGHT, i), besselCtrl(2*WALK_EXTENDED_Z, WALK_EXTENDED_Z, i));
    singleLegCtrl(4,  -WALK_EXTENDED_X, besselCtrl(WALK_HEIGHT_MIN-10, WALK_HEIGHT, i), besselCtrl(2*WALK_EXTENDED_Z, WALK_EXTENDED_Z, i));

    GoalPosAll();
    delay(1);
  }
}


void functionJump(){
  for(float i = 0; i<=1; i+=0.02){
    singleLegCtrl(1, WALK_EXTENDED_X, besselCtrl(WALK_HEIGHT, WALK_HEIGHT_MIN, i), WALK_EXTENDED_Z);
    singleLegCtrl(2,-WALK_EXTENDED_X, besselCtrl(WALK_HEIGHT, WALK_HEIGHT_MIN, i), WALK_EXTENDED_Z);
    singleLegCtrl(3, WALK_EXTENDED_X, besselCtrl(WALK_HEIGHT, WALK_HEIGHT_MIN, i), WALK_EXTENDED_Z);
    singleLegCtrl(4,-WALK_EXTENDED_X, besselCtrl(WALK_HEIGHT, WALK_HEIGHT_MIN, i), WALK_EXTENDED_Z);
    GoalPosAll();
    delay(1);
  }

  singleLegCtrl(1, WALK_EXTENDED_X, WALK_HEIGHT_MAX, WALK_EXTENDED_Z);
  singleLegCtrl(2,-WALK_EXTENDED_X, WALK_HEIGHT_MAX, WALK_EXTENDED_Z);
  singleLegCtrl(3, WALK_EXTENDED_X, WALK_HEIGHT_MAX, WALK_EXTENDED_Z);
  singleLegCtrl(4,-WALK_EXTENDED_X, WALK_HEIGHT_MAX, WALK_EXTENDED_Z);
  GoalPosAll();
  delay(70);

  for(float i = 0; i<=1; i+=0.02){
    singleLegCtrl(1, WALK_EXTENDED_X, besselCtrl(WALK_HEIGHT_MIN, WALK_HEIGHT, i), WALK_EXTENDED_Z);
    singleLegCtrl(2,-WALK_EXTENDED_X, besselCtrl(WALK_HEIGHT_MIN, WALK_HEIGHT, i), WALK_EXTENDED_Z);
    singleLegCtrl(3, WALK_EXTENDED_X, besselCtrl(WALK_HEIGHT_MIN, WALK_HEIGHT, i), WALK_EXTENDED_Z);
    singleLegCtrl(4,-WALK_EXTENDED_X, besselCtrl(WALK_HEIGHT_MIN, WALK_HEIGHT, i), WALK_EXTENDED_Z);
    GoalPosAll();
    delay(1);
  }
}


void functionActionA(){
  WALK_LIFT = 9;
  WALK_LIFT_PROP = 0.25;
  STEP_ITERATE = 0.04;
  GAIT_TYPE = 0;
}


void functionActionB(){
  WALK_LIFT = 9;
  WALK_LIFT_PROP=0.18;
  STEP_ITERATE = 0.01;
  GAIT_TYPE = 1;
}


void functionActionC(){
  Serial.println("C");
}

// 双手拜年动作 - 同时抬起两条前腿并做拜年动作
void functionNewYear(){
  // 第一阶段：准备姿势，完全复制握手动作的前腿抬起方式
  for(float i = 0; i<=1; i+=0.02){
    // 前腿X从WALK_EXTENDED_X减小到0，Y抬高，Z从WALK_EXTENDED_Z变为负值(-15)，这是关键！
    singleLegCtrl(1, besselCtrl(WALK_EXTENDED_X, 0, i), 
                    besselCtrl(WALK_HEIGHT, WALK_HEIGHT_MAX, i), 
                    besselCtrl(WALK_EXTENDED_Z, -15, i));
    singleLegCtrl(3, besselCtrl(WALK_EXTENDED_X, 0, i), 
                    besselCtrl(WALK_HEIGHT, WALK_HEIGHT_MAX, i), 
                    besselCtrl(WALK_EXTENDED_Z, -15, i));

    // 后腿更低更靠后，增强支撑
    singleLegCtrl(2, -WALK_EXTENDED_X, 
                    besselCtrl(WALK_HEIGHT, WALK_HEIGHT_MIN-10, i), 
                    besselCtrl(WALK_EXTENDED_Z, 2*WALK_EXTENDED_Z, i));
    singleLegCtrl(4, -WALK_EXTENDED_X, 
                    besselCtrl(WALK_HEIGHT, WALK_HEIGHT_MIN-10, i), 
                    besselCtrl(WALK_EXTENDED_Z, 2*WALK_EXTENDED_Z, i));

    GoalPosAll();
    delay(5); // 放慢速度，增加稳定性
  }

  delay(200); // 稳定一下

  // 第二阶段：前腿高抬并收回 - 参考握手动作第二步
  // 这一步是关键差异：在握手动作中，抬起的腿（3号腿）X值从0变成很大(WALK_RANGE/2+WALK_EXTENDED_X)
  // 我们这里让两条前腿都做类似的动作，但保持中心对称
  for(float i = 0; i<=1; i+=0.02){
    // 两条前腿X向外侧抬起，Y降低，Z保持
    singleLegCtrl(1, besselCtrl(0, -20, i), 
                    besselCtrl(WALK_HEIGHT_MAX, WALK_HEIGHT_MIN, i), 
                    -15);
    singleLegCtrl(3, besselCtrl(0, 20, i), 
                    besselCtrl(WALK_HEIGHT_MAX, WALK_HEIGHT_MIN, i), 
                    -15);

    GoalPosAll();
    delay(5);
  }

  delay(200); // 稳定一下

  // 第三阶段：拜年动作 - 重复3次
  for(int count = 0; count < 3; count++){
    // 拜下去 - 前肢下压但保持离地
    for(float i = 0; i<=1; i+=0.03){
      singleLegCtrl(1, -20, 
                      besselCtrl(WALK_HEIGHT_MIN, WALK_HEIGHT_MIN+20, i), 
                      -15);
      singleLegCtrl(3, 20, 
                      besselCtrl(WALK_HEIGHT_MIN, WALK_HEIGHT_MIN+20, i), 
                      -15);
      GoalPosAll();
      delay(5);
    }
    
    // 抬起来
    for(float i = 0; i<=1; i+=0.03){
      singleLegCtrl(1, -20, 
                      besselCtrl(WALK_HEIGHT_MIN+20, WALK_HEIGHT_MIN, i), 
                      -15);
      singleLegCtrl(3, 20, 
                      besselCtrl(WALK_HEIGHT_MIN+20, WALK_HEIGHT_MIN, i), 
                      -15);
      GoalPosAll();
      delay(5);
    }
    
    delay(100); // 短暂停顿
  }
  
  // 第四阶段：恢复过程 - 按照握手动作恢复方式
  for(float i = 0; i<=1; i+=0.02){
    // 前腿恢复
    singleLegCtrl(1, besselCtrl(-20, WALK_EXTENDED_X, i), 
                    besselCtrl(WALK_HEIGHT_MIN, WALK_HEIGHT, i), 
                    besselCtrl(-15, WALK_EXTENDED_Z, i));
    singleLegCtrl(3, besselCtrl(20, WALK_EXTENDED_X, i), 
                    besselCtrl(WALK_HEIGHT_MIN, WALK_HEIGHT, i), 
                    besselCtrl(-15, WALK_EXTENDED_Z, i));

    // 后腿恢复
    singleLegCtrl(2, -WALK_EXTENDED_X, 
                    besselCtrl(WALK_HEIGHT_MIN-10, WALK_HEIGHT, i), 
                    besselCtrl(2*WALK_EXTENDED_Z, WALK_EXTENDED_Z, i));
    singleLegCtrl(4, -WALK_EXTENDED_X, 
                    besselCtrl(WALK_HEIGHT_MIN-10, WALK_HEIGHT, i), 
                    besselCtrl(2*WALK_EXTENDED_Z, WALK_EXTENDED_Z, i));

    GoalPosAll();
    delay(5);
  }
}


// base move ctrl.
void robotCtrl(){
  // 添加全局状态监控
  static unsigned long lastStatusCheck = 0;
  static int lastMoveFB = 0;
  static int lastMoveLR = 0;
  static float lastGestureUD = 0;
  static float lastGestureLR = 0;
  static int transitionCounter = 0;
  const int TRANSITION_STEPS = 5; // 过渡步数
  
  // 检测移动或姿态状态变化
  bool movementChanged = (lastMoveFB != moveFB || lastMoveLR != moveLR);
  bool gestureChanged = (abs(lastGestureUD - gestureUD) > 0.5 || abs(lastGestureLR - gestureLR) > 0.5);
  
  // 如果状态变化，启动过渡计数器
  if (movementChanged || gestureChanged) {
    transitionCounter = TRANSITION_STEPS;
  }
  
  // 记录当前状态用于下次比较
  lastMoveFB = moveFB;
  lastMoveLR = moveLR;
  lastGestureUD = gestureUD;
  lastGestureLR = gestureLR;
  
  // 移动控制
  if(!debugMode && !funcMode){
    if(moveFB == 0 && moveLR == 0 && STAND_STILL == 0){
      // 从移动状态过渡到站立状态
      if (transitionCounter > 0) {
        // 计算过渡中的姿态，逐渐回到中心位置
        float transitionRate = transitionCounter / (float)TRANSITION_STEPS;
        float transitGestureUD = gestureUD * transitionRate;
        float transitGestureLR = gestureLR * transitionRate;
        
        pitchYawRollHeightCtrl(transitGestureUD, transitGestureLR, 0, 0);
        transitionCounter--;
      } else {
        standMassCenter(0, 0);
      }
      GoalPosAll();
      STAND_STILL = 1;
      GLOBAL_STEP = 0;
      delay(STEP_DELAY);
    }
    else if(moveFB == 0 && moveLR == 0 && STAND_STILL == 1){
      // 站立状态，处理姿态控制
      if (abs(gestureUD) > 0.1 || abs(gestureLR) > 0.1) {
        pitchYawRollHeightCtrl(gestureUD, gestureLR, 0, 0);
      }
      GoalPosAll();
      delay(STEP_DELAY);
    }
    else{
      STAND_STILL = 0;
      
      // 在移动过程中添加轻微的倾斜补偿，增强稳定性
      float moveCompUD = 0;
      float moveCompLR = 0;
      
      // 前后移动时的轻微倾斜补偿
      if (moveFB != 0) {
        moveCompUD = moveFB * 2.0; // 前进时略微抬头，后退时略微低头
      }
      
      // 左右移动时的轻微倾斜补偿
      if (moveLR != 0) {
        moveCompLR = moveLR * 1.5; // 向左转时略微左倾，向右转时略微右倾
      }
      
      // 结合用户的姿态控制和移动补偿
      float finalGestureUD = gestureUD + moveCompUD;
      float finalGestureLR = gestureLR + moveCompLR;
      
      // 限制最终姿态角度
      if (finalGestureUD > gestureOffSetMax) finalGestureUD = gestureOffSetMax;
      if (finalGestureUD < -gestureOffSetMax) finalGestureUD = -gestureOffSetMax;
      if (finalGestureLR > gestureOffSetMax) finalGestureLR = gestureOffSetMax;
      if (finalGestureLR < -gestureOffSetMax) finalGestureLR = -gestureOffSetMax;
      
      // 应用姿态控制
      pitchYawRollHeightCtrl(finalGestureUD, finalGestureLR, 0, 0);
      
      // 步态控制
      if(GLOBAL_STEP > 1){GLOBAL_STEP = 0;}
      if(moveFB == 1 && moveLR == 0){gaitTypeCtrl(GLOBAL_STEP, 0, 0);}
      else if(moveFB == -1 && moveLR == 0){gaitTypeCtrl(GLOBAL_STEP, 180, 0);}
      else if(moveFB == 1 && moveLR == -1){gaitTypeCtrl(GLOBAL_STEP, 30, 0);}
      else if(moveFB == 1 && moveLR == 1){gaitTypeCtrl(GLOBAL_STEP, -30, 0);}
      else if(moveFB == -1 && moveLR == 1){gaitTypeCtrl(GLOBAL_STEP, -120, 0);}
      else if(moveFB == -1 && moveLR == -1){gaitTypeCtrl(GLOBAL_STEP, 120, 0);}
      else if(moveFB == 0 && moveLR == -1){gaitTypeCtrl(GLOBAL_STEP, 0, -1);}
      else if(moveFB == 0 && moveLR == 1){gaitTypeCtrl(GLOBAL_STEP, 0, 1);}
      GoalPosAll();
      GLOBAL_STEP += STEP_ITERATE;
      delay(STEP_DELAY);
    }
  }
  // function ctrl.
  else if(!debugMode && funcMode){
    if(funcMode == 1){
      accXYZUpdate();
      balancing();
      GoalPosAll();
    }
    else if (funcMode == 2){
      Serial.println("stayLow");
      functionStayLow();
      funcMode = 0;
    }
    else if (funcMode == 3){
      Serial.println("handshake");
      functionHandshake();
      funcMode = 0;
    }
    else if (funcMode == 4){
      Serial.println("Jump");
      functionJump();
      funcMode = 0;
    }
    else if (funcMode == 5){
      Serial.println("ActionA");
      functionActionA();
      funcMode = 0;
    }
    else if (funcMode == 6){
      Serial.println("ActionB");
      functionActionB();
      funcMode = 0;
    }
    else if (funcMode == 7){
      Serial.println("ActionC");
      functionActionC();
      funcMode = 0;
    }
    else if (funcMode == 8){
      Serial.println("InitPos");
      initPosAll();
    }
    else if (funcMode == 9){
      Serial.println("MiddlePos");
      middlePosAll();
    }
    else if (funcMode == 10){
      Serial.println("NewYear");
      functionNewYear();
      funcMode = 0;
    }
    setSingleLED(0,matrix.Color(0, 128, 255));
    setSingleLED(1,matrix.Color(0, 128, 255));
  }
  else if(debugMode){
    setSingleLED(0,matrix.Color(255, 64, 0));
    setSingleLED(1,matrix.Color(255, 64, 0));
    delay(100);
  }
  
  // 每10秒进行一次状态监测，检查传感器和运动状态是否正常
  if (millis() - lastStatusCheck > 10000) {
    // 只在非调试模式下执行
    if (!debugMode) {
      // 读取传感器数据
      accXYZUpdate();
      float accelMagnitude = sqrt(ACC_X*ACC_X + ACC_Y*ACC_Y + ACC_Z*ACC_Z);
      
      // 检测异常情况 - 加速度大小明显偏离重力加速度
      if (accelMagnitude < 0.5 || accelMagnitude > 1.5) {
        Serial.print("异常加速度检测: ");
        Serial.println(accelMagnitude);
        
        // 如果检测到异常，临时进入平衡模式修正
        if (funcMode != 1) {
          Serial.println("执行一次微调恢复");
          int prevFuncMode = funcMode;
          funcMode = 1; // 临时进入平衡模式
          
          // 执行几次平衡计算
          for (int i = 0; i < 5; i++) {
            accXYZUpdate();
            balancing();
            GoalPosAll();
            delay(50);
          }
          
          // 恢复原来的功能模式
          funcMode = prevFuncMode;
        }
      }
    }
    lastStatusCheck = millis();
  }
}


// if the level of IO12 is HIGH, go into debugMode.
void wireDebugDetect(){
  if(digitalRead(WIRE_DEBUG) == HIGH){
    initPosAll();
    debugMode = 1;

    setSingleLED(0,matrix.Color(255, 64, 0));
    setSingleLED(1,matrix.Color(255, 64, 0));

    while(digitalRead(WIRE_DEBUG) == HIGH){
      delay(100);
    }
    delay(1000);
  }
}

#endif // SERVO_CTRL_H