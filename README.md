# 🚀 Sorter_Project

Arduino MEGA + HuskyLens QR Sorting Project  
로봇 소터 시스템의 두 가지 주요 코드 저장소입니다.  

---

## 📂 코드 구성

전체적인 코드

#include <Wire.h>
#include "HUSKYLENS.h"
#include <Servo.h>

// ====== 핀맵 ======
const int B1_EN_R=26, B1_EN_L=27, B1_RPWM=6;          // Belt1
const int B2_EN_R=24, B2_EN_L=25, B2_RPWM=10;         // Belt2
const int B2_LPWM_UNUSED=46;                          // LOW 고정

const int L1_EN_R=12, L1_EN_L=13, L1_RPWM=9,  L1_LPWM=11; 
const int L2_EN_R=22, L2_EN_L=23, L2_RPWM=3,  L2_LPWM=4;  

const int PS1=30, PS2=31, PS3=32, PS4=33, PS5=34;    // 포토센서

const int LED_R=37, LED_G=38;                        
const int SERVO_PIN=28;                              
const int SERVO_CENTER = 100;      
const int SERVO_SWING_DEG = 60;    
const int SERVO_STEP_MS = 80;      

// ====== 파라미터 ======
const uint8_t SPEED_BELT=230, SPEED_LIN=230;
const unsigned T_BELT1_A=4000;
const unsigned T_BELT1_B=4000;
const unsigned T_BELT2_ID1=1100;  
const unsigned T_BELT2_ID2=2225;  
const unsigned T_BELT2_ID3=5000;  
const unsigned T_LIN_FWD=5000, T_LIN_REV=5000, T_DIR_PAUSE=150;
const unsigned DEBOUNCE_MS=50, LOCKOUT_MS=1200;

// ====== 전역 ======
HUSKYLENS husky;
Servo gate;
int lastQR=0; 

// ====== 유틸 ======
void ledOff(){ digitalWrite(LED_R,LOW); digitalWrite(LED_G,LOW); }
void ledBlue3s(){ digitalWrite(LED_R,LOW); digitalWrite(LED_G,HIGH); delay(3000); ledOff(); }
void ledRedBlink4x(){ for(int i=0;i<4;i++){ digitalWrite(LED_R,HIGH); digitalWrite(LED_G,LOW); delay(400); digitalWrite(LED_R,LOW); delay(400);} ledOff(); }

// 기존 안정 판정 (PS1, PS2용만 사용)
bool stableLow(int pin){ const int N=25; int low=0; for(int i=0;i<N;i++){ if(digitalRead(pin)==LOW) low++; delay(2);} return (low>=23); }
bool stableHigh(int pin){ const int N=25; int hi=0; for(int i=0;i<N;i++){ if(digitalRead(pin)==HIGH) hi++; delay(2);} return (hi>=23); }

// 빠른 감지 전용 (PS3~PS5)
bool fastLow(int pin){ return (digitalRead(pin)==LOW); }

// 특정 센서 LOW 대기 (PS1, PS2는 안정판정 사용)
bool waitDetectLow(int pin){
  unsigned long lastTrig=0; bool last=false;
  while(true){
    bool s=stableLow(pin); unsigned long now=millis();
    if(s!=last && (now-lastTrig)>=LOCKOUT_MS){ last=s; lastTrig=now; if(s) return true; }
    delay(2);
  }
}

// 모든 핀 HIGH 대기
bool waitAllClear_Px(const int pins[], int cnt){
  unsigned long lastOK=0;
  while(true){
    bool allClear=true;
    for(int i=0;i<cnt;i++){ if(!stableHigh(pins[i])){ allClear=false; break; } }
    if(allClear){
      if(lastOK==0) lastOK=millis();
      if(millis()-lastOK >= 50) return true;
    } else lastOK=0;
    delay(5);
  }
}

// 바닥쪽 3개 중 하나라도 감지 (빠른 감지)
bool anyPS345(){ return fastLow(PS3) || fastLow(PS4) || fastLow(PS5); }

// 모터 통합 정지
void allStop(){ belt1_off(); belt2_off(); linear1_stop(); linear2_stop(); ledOff(); }

// ====== 구동기 제어 ======
inline void belt1_on(){ digitalWrite(B1_EN_R,HIGH); digitalWrite(B1_EN_L,HIGH); analogWrite(B1_RPWM,SPEED_BELT); }
inline void belt1_off(){ analogWrite(B1_RPWM,0); digitalWrite(B1_EN_R,LOW); digitalWrite(B1_EN_L,LOW); }
inline void belt2_on(){ digitalWrite(B2_EN_R,HIGH); digitalWrite(B2_EN_L,HIGH); analogWrite(B2_RPWM,SPEED_BELT); }
inline void belt2_off(){ analogWrite(B2_RPWM,0); digitalWrite(B2_EN_R,LOW); digitalWrite(B2_EN_L,LOW); }

void linear1_forward(){ digitalWrite(L1_EN_R,HIGH); digitalWrite(L1_EN_L,HIGH); analogWrite(L1_RPWM,SPEED_LIN); analogWrite(L1_LPWM,0); }
void linear1_reverse(){ analogWrite(L1_RPWM,0); analogWrite(L1_LPWM,SPEED_LIN); }
void linear1_stop(){ analogWrite(L1_RPWM,0); analogWrite(L1_LPWM,0); digitalWrite(L1_EN_R,LOW); digitalWrite(L1_EN_L,LOW); }

void linear2_forward(){ digitalWrite(L2_EN_R,HIGH); digitalWrite(L2_EN_L,HIGH); analogWrite(L2_RPWM,SPEED_LIN); analogWrite(L2_LPWM,0); }
void linear2_reverse(){ analogWrite(L2_RPWM,0); analogWrite(L2_LPWM,SPEED_LIN); }
void linear2_stop(){ analogWrite(L2_RPWM,0); analogWrite(L2_LPWM,0); digitalWrite(L2_EN_R,LOW); digitalWrite(L2_EN_L,LOW); }

// ====== 서보 + QR ======
bool servoScanForQR(int cycles,int center,int swingDeg,int stepMs){
  int half=swingDeg/2, a=center-half, b=center+half;
  gate.write(center); delay(150);
  for(int k=0; k<cycles; k++){
    for(int d=center; d<=b; d++){ gate.write(d); delay(stepMs);
      if(husky.request() && husky.available()){
        while(husky.available()){
          HUSKYLENSResult r=husky.read();
          if(r.ID>=1 && r.ID<=3){ lastQR=r.ID; gate.write(center); return true; }
        }
      }
    }
    for(int d=b; d>=a; d--){ gate.write(d); delay(stepMs);
      if(husky.request() && husky.available()){
        while(husky.available()){
          HUSKYLENSResult r=husky.read();
          if(r.ID>=1 && r.ID<=3){ lastQR=r.ID; gate.write(center); return true; }
        }
      }
    }
    for(int d=a; d<=center; d++){ gate.write(d); delay(stepMs);
      if(husky.request() && husky.available()){
        while(husky.available()){
          HUSKYLENSResult r=husky.read();
          if(r.ID>=1 && r.ID<=3){ lastQR=r.ID; gate.write(center); return true; }
        }
      }
    }
  }
  gate.write(center);
  return false;
}

// ====== 바닥 센서 감지 → 정지 → 클리어 ======
void waitAnyBottomThenRearm(){
  Serial.println(F("[SEQ] Waiting bottom sensor (PS3/4/5)..."));
  while(!anyPS345()) delay(1);  // 빠른 감지 모드 (1ms)
  Serial.println(F("[SEQ] Bottom detected → REARM"));

  allStop();                                   
  const int END_PINS[]  = {PS3, PS4, PS5};     
  const int TRIG_PINS[] = {PS1, PS2};          
  waitAllClear_Px(END_PINS, 3);
  waitAllClear_Px(TRIG_PINS, 2);

  Serial.println(F("[REARM] All clear. Ready for next block (PS1)."));
}

// ====== 사이클 실행 ======
void runOneCycle(){
  waitDetectLow(PS1);
  Serial.println(F("[SEQ] PS1 detected → Belt1 4s"));
  belt1_on(); delay(T_BELT1_A); belt1_off(); delay(150);

  lastQR=0;
  Serial.println(F("[SEQ] Servo swing + QR scan..."));
  bool got = servoScanForQR(3, SERVO_CENTER, SERVO_SWING_DEG, SERVO_STEP_MS);
  if (got){ Serial.print(F("[QR] ID=")); Serial.println(lastQR); }
  else    { Serial.println(F("[QR] none → treat as ID=3")); lastQR = 3; }

  if (lastQR==1 || lastQR==2){ Serial.println(F("[LED] GREEN 3s")); ledBlue3s(); }
  else                       { Serial.println(F("[LED] RED blink x4")); ledRedBlink4x(); }

  Serial.println(F("[SEQ] Belt1 4s"));
  belt1_on(); delay(T_BELT1_B); belt1_off();

  Serial.println(F("[SEQ] Waiting PS2..."));
  waitDetectLow(PS2);

  if (lastQR==1){
    Serial.println(F("[SEQ] QR=1 → Belt2 + L1"));
    belt2_on(); delay(T_BELT2_ID1); belt2_off(); delay(150);
    linear1_forward(); delay(T_LIN_FWD);
    linear1_reverse(); delay(T_LIN_REV);
    linear1_stop();
  } else if (lastQR==2){
    Serial.println(F("[SEQ] QR=2 → Belt2 + L2"));
    belt2_on(); delay(T_BELT2_ID2); belt2_off(); delay(150);
    linear2_forward(); delay(T_LIN_FWD);
    linear2_reverse(); delay(T_LIN_REV);
    linear2_stop();
  } else {
    Serial.println(F("[SEQ] QR=3 → Belt2 only"));
    belt2_on(); delay(T_BELT2_ID3); belt2_off();
  }

  waitAnyBottomThenRearm();
}

void setup(){
  Serial.begin(115200);

  pinMode(B1_EN_R,OUTPUT); pinMode(B1_EN_L,OUTPUT); pinMode(B1_RPWM,OUTPUT);
  pinMode(B2_EN_R,OUTPUT); pinMode(B2_EN_L,OUTPUT); pinMode(B2_RPWM,OUTPUT);
  pinMode(B2_LPWM_UNUSED,OUTPUT); digitalWrite(B2_LPWM_UNUSED,LOW);

  pinMode(L1_EN_R,OUTPUT); pinMode(L1_EN_L,OUTPUT); pinMode(L1_RPWM,OUTPUT); pinMode(L1_LPWM,OUTPUT);
  pinMode(L2_EN_R,OUTPUT); pinMode(L2_EN_L,OUTPUT); pinMode(L2_RPWM,OUTPUT); pinMode(L2_LPWM,OUTPUT);

  pinMode(PS1,INPUT_PULLUP); pinMode(PS2,INPUT_PULLUP);
  pinMode(PS3,INPUT_PULLUP); pinMode(PS4,INPUT_PULLUP); pinMode(PS5,INPUT_PULLUP);

  pinMode(LED_R,OUTPUT); pinMode(LED_G,OUTPUT); ledOff();

  gate.attach(SERVO_PIN); gate.write(SERVO_CENTER);

  Wire.begin();
  if(!husky.begin(Wire)) Serial.println(F("Husky init failed (I2C)."));
  else                   Serial.println(F("Husky ready."));

  allStop();
  Serial.println(F("Ready. Put a block; waiting PS1..."));
}

void loop(){
  runOneCycle();
}

| 구분 | 설명 | 링크 |
|------|------|------|
| 🎯 Tilt + QR 코드 | 서보 모터로 QR을 스캔하고, 인식된 ID(1~3)에 따라 동작 제어 | [열기](./tilt_qr/tilt_qr.ino) |
| ⚙️ Conveyor Only | QR 없이 강제로 분류 ID를 설정하여 테스트 | [열기](./conveyor_only/conveyor_only.ino) |

---

## 🧠 참고 사항
- **보드:** Arduino MEGA 2560  
- **비전 센서:** HuskyLens (QR 인식)  
- **구동 장치:** 컨베이어 벨트 2개 + 리니어 액추에이터 2개 + 서보 모터 1개  
- **전원:** 24V SMPS (Belt/Linear) / 5V (서보, HuskyLens)

---

## 📸 예시 이미지 (선택 사항)
원한다면 나중에 시스템 사진이나 배선도 이미지도 추가할 수 있습니다.

---

> 🔗 작성자: aka6866asdzzxc-maker  
> 🕓 최신 수정: 2025-10-10
