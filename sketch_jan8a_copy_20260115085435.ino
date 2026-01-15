#include <Wire.h>
#include "MAX30105.h"

MAX30105 particleSensor;

// --- CẤU HÌNH ---
const int LED_PIN = 12;
const int BUZZER_PIN = 11;
const int MPU_ADDR = 0x68;

// --- TINH CHỈNH NGƯỠNG (QUAN TRỌNG) ---
const long NGUONG_CO_TAY = 40000;
const int NGUONG_RUNG_MANH = 5000;     // Tăng lên 5000 (Bơi mạnh cũng khó chạm)
const int GIOI_HAN_HOANG_LOAN = 25;    // Tăng lên 25 (Cần vùng vẫy lâu hơn mới báo)
const unsigned long THOI_GIAN_BAT_DONG = 15000; 

int16_t old_X, old_Y, old_Z;
unsigned long last_move_time = 0;
int diem_hoang_loan = 0; 

void setup() {
  Serial.begin(9600);
  Wire.begin();
  
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  Wire.beginTransmission(MPU_ADDR); Wire.write(0x6B); Wire.write(0); Wire.endTransmission(true);

  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) {
    Serial.println("Loi MAX30102"); 
    digitalWrite(BUZZER_PIN, HIGH); delay(2000); digitalWrite(BUZZER_PIN, LOW);
    while (1);
  }
  particleSensor.setup(); 
  particleSensor.setPulseAmplitudeRed(0x0A);
  
  Wire.beginTransmission(MPU_ADDR); Wire.write(0x3B); Wire.endTransmission(false);
  Wire.requestFrom(MPU_ADDR, 6, true);
  old_X = Wire.read()<<8 | Wire.read();
  old_Y = Wire.read()<<8 | Wire.read();
  old_Z = Wire.read()<<8 | Wire.read();

  // Bíp khởi động
  for(int i=0; i<2; i++) {
    digitalWrite(BUZZER_PIN, HIGH); digitalWrite(LED_PIN, HIGH); delay(100); 
    digitalWrite(BUZZER_PIN, LOW); digitalWrite(LED_PIN, LOW); delay(100);
  }
}

void canh_bao_SOS() {
  digitalWrite(LED_PIN, HIGH); digitalWrite(BUZZER_PIN, HIGH); delay(150); 
  digitalWrite(LED_PIN, LOW); digitalWrite(BUZZER_PIN, LOW); delay(150); 
  digitalWrite(LED_PIN, HIGH); digitalWrite(BUZZER_PIN, HIGH); delay(150); 
  digitalWrite(LED_PIN, LOW); digitalWrite(BUZZER_PIN, LOW); delay(150);
}

void loop() {
  long irValue = particleSensor.getIR();
  
  Wire.beginTransmission(MPU_ADDR); Wire.write(0x3B); Wire.endTransmission(false);
  Wire.requestFrom(MPU_ADDR, 6, true);
  int16_t AcX = Wire.read()<<8 | Wire.read();
  int16_t AcY = Wire.read()<<8 | Wire.read();
  int16_t AcZ = Wire.read()<<8 | Wire.read();

  long luc_rung = abs(AcX - old_X) + abs(AcY - old_Y) + abs(AcZ - old_Z);
  old_X = AcX; old_Y = AcY; old_Z = AcZ;

  // --- LOGIC PHÂN BIỆT BƠI VS VÙNG VẪY ---
  if (luc_rung > NGUONG_RUNG_MANH) {
    diem_hoang_loan++; // Nếu rung mạnh: +1 điểm
    last_move_time = millis(); 
  } else {
    // Nếu nghỉ (lướt nước): Trừ hẳn 2 điểm (để dập tắt báo động giả nhanh hơn)
    if (diem_hoang_loan > 0) diem_hoang_loan = diem_hoang_loan - 2;
    if (diem_hoang_loan < 0) diem_hoang_loan = 0;
  }

  // Logic Bất động
  if (luc_rung > 500 && luc_rung < NGUONG_RUNG_MANH) {
    last_move_time = millis(); 
  }
  
  bool mat_tin_hieu = (irValue < NGUONG_CO_TAY);
  bool bi_chim      = (millis() - last_move_time > THOI_GIAN_BAT_DONG);
  bool dang_vung_vay = (diem_hoang_loan > GIOI_HAN_HOANG_LOAN);

  Serial.print("Rung: "); Serial.print(luc_rung);
  Serial.print(" | Diem: "); Serial.print(diem_hoang_loan);

  if (mat_tin_hieu) {
    Serial.println(" -> NGUY HIEM: ROT PHAO!");
    canh_bao_SOS();
  } 
  else if (bi_chim) {
    Serial.println(" -> NGUY HIEM: CHIM/BAT DONG!");
    canh_bao_SOS();
  }
  else if (dang_vung_vay) {
    Serial.println(" -> NGUY HIEM: VUNG VAY!");
    canh_bao_SOS();
  }
  else {
    Serial.println(" -> An toan");
    digitalWrite(LED_PIN, LOW);
    digitalWrite(BUZZER_PIN, LOW);
  }
  
  delay(100); 
}