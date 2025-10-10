/*
==========================================
 Arduino MEGA + HuskyLens QR Test Code
==========================================
 기능:
  - 서보 스윙으로 QR 코드 탐색
  - 인식된 ID(1~3)에 따라 LED 색상 표시
  - QR 인식 실패 시 빨간 LED 점멸

 작성자: aka6866asdzxc-maker
==========================================
*/

#include <Wire.h>
#include "HUSKYLENS.h"
#include <Servo.h>

// ====== 핀 설정 ======
const int LED_R = 37;   // 빨강 LED
const int LED_G = 38;   // 초록 (파랑 대체)
const int SERVO_PIN = 28; // 서보 제어핀

// ====== 서보 파라미터 ======
const int SERVO_CENTER = 100;    // 중앙 각도
const int SERVO_SWING_DEG = 60;  // 좌우 회전 범위
const int SERVO_STEP_MS = 80;    // 이동 속도 (ms/step)
const int SCAN_CYCLES = 3;       // 탐색 반복 횟수

// ====== 전역 변수 ======
HUSKYLENS husky;
Servo gate;
int lastQR = 0;

// ====== LED 제어 ======
void ledOff() {
  digitalWrite(LED_R, LOW);
  digitalWrite(LED_G, LOW);
}

void ledBlue3s() { // QR 인식 성공
  digitalWrite(LED_R, LOW);
  digitalWrite(LED_G, HIGH);
  delay(3000);
  ledOff();
}

void ledRedBlink4x() { // 인식 실패
  for (int i = 0; i < 4; i++) {
    digitalWrite(LED_R, HIGH);
    digitalWrite(LED_G, LOW);
    delay(400);
    digitalWrite(LED_R, LOW);
    delay(400);
  }
  ledOff();
}

// ====== QR 인식 함수 ======
bool checkQR() {
  if (husky.request() && husky.available()) {
    while (husky.available()) {
      HUSKYLENSResult r = husky.read();
      if (r.ID >= 1 && r.ID <= 3) {
        lastQR = r.ID;
        return true;
      }
    }
  }
  return false;
}

// ====== 서보 스윙으로 QR 탐색 ======
bool servoScanForQR(int cycles, int center, int swingDeg, int stepMs) {
  int half = swingDeg / 2;
  int a = center - half;
  int b = center + half;

  gate.write(center);
  delay(200);

  for (int k = 0; k < cycles; k++) {
    // 오른쪽으로 회전
    for (int d = center; d <= b; d++) {
      gate.write(d);
      delay(stepMs);
      if (checkQR()) return true;
    }
    // 왼쪽으로 회전
    for (int d = b; d >= a; d--) {
      gate.write(d);
      delay(stepMs);
      if (checkQR()) return true;
    }
    // 다시 중앙으로 복귀
    for (int d = a; d <= center; d++) {
      gate.write(d);
      delay(stepMs);
      if (checkQR()) return true;
    }
  }

  gate.write(center);
  return false;
}

// ====== 초기화 ======
void setup() {
  Serial.begin(115200);
  pinMode(LED_R, OUTPUT);
  pinMode(LED_G, OUTPUT);
  ledOff();

  gate.attach(SERVO_PIN);
  gate.write(SERVO_CENTER);

  Wire.begin();
  if (!husky.begin(Wire)) {
    Serial.println(F("⚠️ HuskyLens 초기화 실패 (I2C)"));
  } else {
    Serial.println(F("✅ HuskyLens 연결 완료"));
  }

  Serial.println(F("=== Tilt+QR Scan Ready ==="));
}

// ====== 메인 루프 ======
void loop() {
  lastQR = 0;
  bool got = servoScanForQR(SCAN_CYCLES, SERVO_CENTER, SERVO_SWING_DEG, SERVO_STEP_MS);

  if (got) {
    Serial.print(F("[QR] 인식 성공 → ID="));
    Serial.println(lastQR);

    if (lastQR == 1 || lastQR == 2) ledBlue3s();
    else ledRedBlink4x();

  } else {
    Serial.println(F("[QR] 인식 실패"));
    ledRedBlink4x();
  }

  delay(1000); // 다음 스캔까지 대기
}
/*
==========================================
 Arduino MEGA + HuskyLens QR Test Code
==========================================
 기능:
  - 서보 스윙으로 QR 코드 탐색
  - 인식된 ID(1~3)에 따라 LED 색상 표시
  - QR 인식 실패 시 빨간 LED 점멸

 작성자: aka6866asdzxc-maker
==========================================
*/

#include <Wire.h>
#include "HUSKYLENS.h"
#include <Servo.h>

// ====== 핀 설정 ======
const int LED_R = 37;   // 빨강 LED
const int LED_G = 38;   // 초록 (파랑 대체)
const int SERVO_PIN = 28; // 서보 제어핀

// ====== 서보 파라미터 ======
const int SERVO_CENTER = 100;    // 중앙 각도
const int SERVO_SWING_DEG = 60;  // 좌우 회전 범위
const int SERVO_STEP_MS = 80;    // 이동 속도 (ms/step)
const int SCAN_CYCLES = 3;       // 탐색 반복 횟수

// ====== 전역 변수 ======
HUSKYLENS husky;
Servo gate;
int lastQR = 0;

// ====== LED 제어 ======
void ledOff() {
  digitalWrite(LED_R, LOW);
  digitalWrite(LED_G, LOW);
}

void ledBlue3s() { // QR 인식 성공
  digitalWrite(LED_R, LOW);
  digitalWrite(LED_G, HIGH);
  delay(3000);
  ledOff();
}

void ledRedBlink4x() { // 인식 실패
  for (int i = 0; i < 4; i++) {
    digitalWrite(LED_R, HIGH);
    digitalWrite(LED_G, LOW);
    delay(400);
    digitalWrite(LED_R, LOW);
    delay(400);
  }
  ledOff();
}

// ====== QR 인식 함수 ======
bool checkQR() {
  if (husky.request() && husky.available()) {
    while (husky.available()) {
      HUSKYLENSResult r = husky.read();
      if (r.ID >= 1 && r.ID <= 3) {
        lastQR = r.ID;
        return true;
      }
    }
  }
  return false;
}

// ====== 서보 스윙으로 QR 탐색 ======
bool servoScanForQR(int cycles, int center, int swingDeg, int stepMs) {
  int half = swingDeg / 2;
  int a = center - half;
  int b = center + half;

  gate.write(center);
  delay(200);

  for (int k = 0; k < cycles; k++) {
    // 오른쪽으로 회전
    for (int d = center; d <= b; d++) {
      gate.write(d);
      delay(stepMs);
      if (checkQR()) return true;
    }
    // 왼쪽으로 회전
    for (int d = b; d >= a; d--) {
      gate.write(d);
      delay(stepMs);
      if (checkQR()) return true;
    }
    // 다시 중앙으로 복귀
    for (int d = a; d <= center; d++) {
      gate.write(d);
      delay(stepMs);
      if (checkQR()) return true;
    }
  }

  gate.write(center);
  return false;
}

// ====== 초기화 ======
void setup() {
  Serial.begin(115200);
  pinMode(LED_R, OUTPUT);
  pinMode(LED_G, OUTPUT);
  ledOff();

  gate.attach(SERVO_PIN);
  gate.write(SERVO_CENTER);

  Wire.begin();
  if (!husky.begin(Wire)) {
    Serial.println(F("⚠️ HuskyLens 초기화 실패 (I2C)"));
  } else {
    Serial.println(F("✅ HuskyLens 연결 완료"));
  }

  Serial.println(F("=== Tilt+QR Scan Ready ==="));
}

// ====== 메인 루프 ======
void loop() {
  lastQR = 0;
  bool got = servoScanForQR(SCAN_CYCLES, SERVO_CENTER, SERVO_SWING_DEG, SERVO_STEP_MS);

  if (got) {
    Serial.print(F("[QR] 인식 성공 → ID="));
    Serial.println(lastQR);

    if (lastQR == 1 || lastQR == 2) ledBlue3s();
    else ledRedBlink4x();

  } else {
    Serial.println(F("[QR] 인식 실패"));
    ledRedBlink4x();
  }

  delay(1000); // 다음 스캔까지 대기
}
