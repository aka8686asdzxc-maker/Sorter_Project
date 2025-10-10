/*
========================================
 Arduino MEGA - Conveyor Only Test Code
----------------------------------------
- QR 없이도 분기 테스트 가능 (FORCED_ID로 선택)
- PS1 감지 → Belt1 이송 → PS2 감지 후 분기 동작
- 바닥(PS3/4/5) 감지되면 정지 후 재무장
========================================
*/

// ====== 핀맵 ======
const int B1_EN_R=26, B1_EN_L=27, B1_RPWM=6;      // Belt1
const int B2_EN_R=24, B2_EN_L=25, B2_RPWM=10;     // Belt2
const int B2_LPWM_UNUSED=46;                      // 사용 안함(LOW 고정)

const int L1_EN_R=12, L1_EN_L=13, L1_RPWM=9,  L1_LPWM=11; 
const int L2_EN_R=22, L2_EN_L=23, L2_RPWM=3,  L2_LPWM=4;

const int PS1=30, PS2=31, PS3=32, PS4=33, PS5=34; // 포토센서 (NPN/OC → LOW)
const int LED_R=37, LED_G=38;                     // 상태 LED(선택)

// ====== 파라미터 ======
const uint8_t SPEED_BELT=230, SPEED_LIN=230;
const unsigned T_BELT1_A=4000, T_BELT1_B=4000;
const unsigned T_BELT2_ID1=1100, T_BELT2_ID2=2225, T_BELT2_ID3=5000;
const unsigned T_LIN_FWD=5000, T_LIN_REV=5000;
const unsigned LOCKOUT_MS=1200;                   // 디바운스/락아웃

// ====== 테스트용 강제 분기 (1/2/3 중 골라서 사용) ======
const int FORCED_ID = 1; // ← 필요에 따라 1, 2, 3으로 바꿔 테스트
int lastQR = FORCED_ID;

// ====== 유틸 ======
void ledOff(){ pinMode(LED_R,OUTPUT); pinMode(LED_G,OUTPUT); digitalWrite(LED_R,LOW); digitalWrite(LED_G,LOW); }
bool stableLow(int pin){ const int N=25; int low=0; for(int i=0;i<N;i++){ if(digitalRead(pin)==LOW) low++; delay(2);} return (low>=23); }
bool stableHigh(int pin){ const int N=25; int hi=0; for(int i=0;i<N;i++){ if(digitalRead(pin)==HIGH) hi++; delay(2);} return (hi>=23); }
bool fastLow(int pin){ return (digitalRead(pin)==LOW); }

bool waitDetectLow(int pin){
  unsigned long lastTrig=0; bool last=false;
  while(true){
    bool s=stableLow(pin); unsigned long now=millis();
    if(s!=last && (now-lastTrig)>=LOCKOUT_MS){ last=s; lastTrig=now; if(s) return true; }
    delay(2);
  }
}

bool waitAllClear(const int pins[], int cnt){
  unsigned long t0=0;
  while(true){
    bool ok=true;
    for(int i=0;i<cnt;i++){ if(!stableHigh(pins[i])){ ok=false; break; } }
    if(ok){ if(t0==0) t0=millis(); if(millis()-t0>=50) return true; }
    else t0=0;
    delay(5);
  }
}

bool anyPS345(){ return fastLow(PS3) || fastLow(PS4) || fastLow(PS5); }

// ====== 모터 제어 ======
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

void allStop(){ belt1_off(); belt2_off(); linear1_stop(); linear2_stop(); ledOff(); }

// ====== 바닥 감지 → 정지 → 재무장 ======
void waitAnyBottomThenRearm(){
  while(!anyPS345()) delay(1);     // 빠르게 감시
  allStop();
  const int END_PINS[]  = {PS3, PS4, PS5};
  const int TRIG_PINS[] = {PS1, PS2};
  waitAllClear(END_PINS, 3);
  waitAllClear(TRIG_PINS, 2);
}

// ====== 사이클 ======
void runOneCycle(){
  // 1) 투입 감지 (PS1)
  waitDetectLow(PS1);
  belt1_on(); delay(T_BELT1_A); belt1_off(); delay(150);

  // 2) (QR 대신) 고정 분기로 설정
  lastQR = FORCED_ID;

  // 3) 추가 이송 후 분기 지점(PS2) 대기
  belt1_on(); delay(T_BELT1_B); belt1_off();
  waitDetectLow(PS2);

  // 4) 분기 동작
  if (lastQR==1){
    belt2_on(); delay(T_BELT2_ID1); belt2_off(); delay(150);
    linear1_forward(); delay(T_LIN_FWD);
    linear1_reverse(); delay(T_LIN_REV);
    linear1_stop();
  } else if (lastQR==2){
    belt2_on(); delay(T_BELT2_ID2); belt2_off(); delay(150);
    linear2_forward(); delay(T_LIN_FWD);
    linear2_reverse(); delay(T_LIN_REV);
    linear2_stop();
  } else {
    belt2_on(); delay(T_BELT2_ID3); belt2_off();
  }

  // 5) 바닥 센서 감지 후 재무장
  waitAnyBottomThenRearm();
}

// ====== 초기화 ======
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

  allStop();
  Serial.println(F("✅ Conveyor-only ready. Waiting PS1..."));
}

void loop(){ runOneCycle(); }
