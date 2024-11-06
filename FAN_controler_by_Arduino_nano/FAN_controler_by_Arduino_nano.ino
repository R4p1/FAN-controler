#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27,16,2);

volatile unsigned long pulseCount = 0;

int i;
int const PWM_pin = 9;
int const SW = 4;
int const RPM_pin = 2;
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 200;

void setup() {
  // Timer1を25kHzに設定
  TCCR1A = 0b10100010; // Fast PWM, non-inverting mode
  TCCR1B = 0b00011001; // Fast PWM mode, no prescaler
  ICR1 = 319; // 25kHz (16MHz / (1 * (319 + 1)))

  pinMode(PWM_pin, OUTPUT);
  pinMode(RPM_pin, INPUT); 
  attachInterrupt(digitalPinToInterrupt(RPM_pin), countPulse, RISING); 
  pinMode(SW, INPUT_PULLUP);
  i = 0;
  lcd.init(); 
  lcd.backlight();
  lcd.setCursor(0, 0);
  Serial.begin(9600);  // デバッグ用シリアル通信の開始
  delay(1000);
}

unsigned long previousMicros = 0;
const unsigned long intervalMicros = 1000000; // 1秒 (1,000,000マイクロ秒)

void loop() {
  unsigned long currentMicros = micros();

  if (currentMicros - previousMicros >= intervalMicros) {
    previousMicros = currentMicros;
    updateLCD();
  }

  performOtherTasks();
}

void updateLCD() {
  lcd.setCursor(8, 1);

  // 割り込みの一時停止
  noInterrupts();
  int pulsesPerRevolution = 2;  // 1回転あたりのパルス数
  unsigned long pulseCountCopy = pulseCount;  // 安全なカウント取得のためにコピー
  pulseCount = 0;  // カウントリセット
  interrupts();  // 割り込みを再開

  // RPMの計算 (1秒あたりのパルス数を60秒に換算)
  long RPM = (pulseCountCopy * 60L) / pulsesPerRevolution; // 60を使用

  // デバッグ用出力
  Serial.print("Pulse Count Copy: ");
  Serial.print(pulseCountCopy);
  Serial.print(" RPM: ");
  Serial.println(RPM);

  // LCDの更新 (必要な範囲だけクリア)
  lcd.setCursor(6, 1);
  lcd.print("       ");  // 前の表示をクリア
  lcd.setCursor(6, 1);
  lcd.print(RPM);
}

void performOtherTasks() {
  updatePWM();
  displayPWM();
}

void updatePWM() {
  // PWMの更新
  if (digitalRead(SW) == LOW && (millis() - lastDebounceTime > debounceDelay)) {
    if (i < 10) {
      i++;
    } else {
      i = 0; // ここでリセット
      lcd.setCursor(6, 0);
      lcd.print("   ");
    }
    lastDebounceTime = millis();
  }

  OCR1A = (i * 319) / 10; // iに基づいてPWMのデューティサイクルを設定
}

void displayPWM() {
  // PWMの表示
  lcd.setCursor(0, 0);
  lcd.print("PWM = ");
  int PWM = i * 10;
  lcd.setCursor(6, 0);
  lcd.print(PWM);
  lcd.setCursor(9, 0);
  lcd.print("%");
  lcd.setCursor(0, 1);
  lcd.print("RPM =");
}

void countPulse() {
  static unsigned long lastPulseTime = 0;
  unsigned long currentTime = micros();
  
  // デバウンス期間を200マイクロ秒に設定
  if (currentTime - lastPulseTime > 200) {
    pulseCount++;
    lastPulseTime = currentTime;
  }
}
