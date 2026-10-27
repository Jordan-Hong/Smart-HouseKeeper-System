#include <Wire.h>
#include <SparkFun_APDS9960.h>
#include <Servo.h>
#include <LiquidCrystal.h>
#include <SoftwareSerial.h>

// --- 腳位配置 ---
#define LED_R 5          
#define LED_G 6          
#define LED_B 11         
#define BUZZER_PIN 2     
#define APDS9960_INT 3   
#define SERVO_PIN 4      
#define LCD_BACKLIGHT A0 

// --- 物件建立 ---
SparkFun_APDS9960 apds = SparkFun_APDS9960();
Servo myServo;
LiquidCrystal lcd(7, 8, 9, 10, A1, A2);
SoftwareSerial bt(12, 13); 

// --- 系統狀態變數 ---
bool isAsleep = true;
bool isLightOn = false;          
bool isCurtainOpen = false;      
uint8_t currentBrightness = 255; 

// --- 系統級防盜與通訊狀態 ---
bool isAlertMode = false;
bool waitingForHeadcount = false;

// --- 時間與感測變數 ---
unsigned long hoverStartTime = 0;
unsigned long lastActiveTime = 0; 
unsigned long lastLcdUpdate = 0;  
unsigned long alertStart = 0;
unsigned long headcountRequestTime = 0; //新增：用來記錄發送 'P' 的時間

bool isHovering = false;
bool wakeTriggered = false;

void setup() {
  Serial.begin(9600); 
  bt.begin(38400);     
  
  pinMode(LED_R, OUTPUT);
  pinMode(LED_G, OUTPUT);
  pinMode(LED_B, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LCD_BACKLIGHT, OUTPUT); 

  setRGB(0, 0, 0);
  myServo.attach(SERVO_PIN);
  myServo.write(95);
  delay(2000); 

  lcd.begin(16, 2);
  goToSleep();

  if (apds.init() && apds.enableProximitySensor(false)) {
    Serial.println("\n[系統就緒] 進入休眠模式。");
    Serial.println("=============================================");
    Serial.println("【藍牙雙機對接 + 離線備援模式】");
    Serial.println("- 連線狀態下：等待 Agent 1 回傳人數，判斷是否防盜。");
    Serial.println("- 離線狀態下：若 1.5 秒無回應，自動切換為獨立管家喚醒！");
    Serial.println("=============================================");
  }
}

void loop() {
  // ==========================================
  // 核心 0：警報模式死鎖
  // ==========================================
  if (isAlertMode) {
    if(millis() - alertStart >= 10000){
      isAlertMode = false;
      digitalWrite(BUZZER_PIN, LOW);
      setRGB(0, 0, 0);
      myServo.write(95);

      isAsleep = true;
      digitalWrite(LCD_BACKLIGHT, LOW); 
      lcd.clear();

      isLightOn = false;          
      isCurtainOpen = false;      
      currentBrightness = 255;
      waitingForHeadcount = false;
      Serial.println("\n[系統提示]：10秒已過，警報解除並恢復休眠。");
    }
    return; 
  }

  // ==========================================
  // 核心 1：藍牙雙機通訊 (Agent 1 <-> Agent 2)
  // ==========================================
  if (bt.available()) {
    char cmd = bt.read();
    Serial.println("\n cmd:");
    Serial.println(cmd); // 換成 println 比較好讀

    if (waitingForHeadcount) {
      if (cmd == '0') {
        Serial.println("\n[藍牙接收] 收到 '0'：0人 (觸發防盜)");
        triggerAlertMode(); 
        alertStart = millis();
      } else if (cmd == '1') {
        Serial.println("\n[藍牙接收] 收到 '1'：1人以上 (連線正常喚醒)");
        setRGB(0,255,0);
        delay(100);
        setRGB(0,0,0);
        executeNormalWakeUp(); 
      }
    }

    if (cmd == 'E' || cmd == 'e') {
      Serial.println("\n[藍牙接收] 收到 'E'：人員已清空，檢查燈具狀態...");
      if (isLightOn) {
        setRGB(0, 0, 0);
        isLightOn = false;
        bt.print('B'); 
        Serial.println("-> 燈亮著，已自動關閉。回傳代碼：B");
        if (!isAsleep) {
          updateLCD();
          isAsleep = true;
          digitalWrite(LCD_BACKLIGHT, LOW); 
          lcd.clear();
        }
      } else {
        bt.print('A'); 
        Serial.println("-> 燈本來就暗的。回傳代碼：A");
        isAsleep = true;
        digitalWrite(LCD_BACKLIGHT, LOW); 
        lcd.clear();
      }
    }
  }

  // 核心 1.5：離線備援機制 (Timeout Fallback)
  // 如果正在等待人數，且等待時間超過 1.5 秒 (1500 毫秒) 都沒收到藍牙回應
  if (waitingForHeadcount && (millis() - headcountRequestTime > 1500)) {
    Serial.println("\n[離線模式啟動] 2秒內未收到 Agent 1 回應，自動獨立喚醒管家！");
    setRGB(0,0,255);
    delay(100);
    setRGB(0,0,0);
    executeNormalWakeUp(); 
  }

  // ==========================================
  // 核心 2：APDS-9960 紅外線狀態機
  // ==========================================
  uint8_t prox = 0;
  if (apds.readProximity(prox)) {
    
    if (prox > 80) {
      lastActiveTime = millis();
    } else {
      if (!isAsleep && (millis() - lastActiveTime > 10000)) {
        goToSleep();
      }
    }

    uint8_t activeThreshold = isAsleep ? 200 : 100;
    
    if (prox > activeThreshold) { 
      if (!isHovering) {
        isHovering = true;
        hoverStartTime = millis();
        wakeTriggered = false;
      } else {
        unsigned long holdTime = millis() - hoverStartTime;
        
        if (isAsleep) {
          if (holdTime > 3000 && !wakeTriggered) {
            wakeTriggered = true;
            waitingForHeadcount = true;
            headcountRequestTime = millis(); // 紀錄發送請求的時間
            
            bt.print('Q'); 
            Serial.println("\n[藍牙發送] 'Q' (請求人數資訊，等待連線回應...)"); 
            shortBeep();
          }
        } 
        else {
          if (!waitingForHeadcount && isLightOn && holdTime > 2000) {
            if (currentBrightness > 0) {
              currentBrightness -= 51;
              setRGB(currentBrightness, currentBrightness, currentBrightness); 
              
              if (millis() - lastLcdUpdate > 500) {
                updateLCD();
                lastLcdUpdate = millis();
              }
              delay(500); 
            }
            
            if (currentBrightness == 0) {
              isLightOn = false;
              updateLCD();
              shortBeep();
              isHovering = false;
            }
          }
        }
      }
    } 
    else { 
      if (isHovering) {
        isHovering = false;
        unsigned long duration = millis() - hoverStartTime;

        if (waitingForHeadcount) {
          waitingForHeadcount = false;
          Serial.println("[系統提示]：使用者提早把手移開，取消等待人數回傳。");
        }

        if (!isAsleep) {
          if (duration > 50 && duration <= 1000) {
            toggleLight();
          } 
          else if (duration > 1000 && duration <= 2000) {
            toggleCurtain();
          }
          else if (duration > 2000) {
            shortBeep();
          }
        }
      }
    }
  }
  delay(15);
}

// ==========================================
// 雙機通訊與系統狀態切換
// ==========================================

void executeNormalWakeUp() {
  waitingForHeadcount = false;
  wakeUp(); 
}

void triggerAlertMode() {
  waitingForHeadcount = false;
  isAlertMode = true; 
  
  digitalWrite(LCD_BACKLIGHT, HIGH); 
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("! ALERT MODE !");
  lcd.setCursor(0, 1);
  lcd.print("INTRUDER DETECT");

  setRGB(255, 0, 0);
  digitalWrite(BUZZER_PIN, HIGH); 
}

void wakeUp() {
  isAsleep = false;
  lastActiveTime = millis(); 
  digitalWrite(LCD_BACKLIGHT, HIGH); 
  updateLCD(); 
  longBeep(3);
}

void goToSleep() {
  isAsleep = true;
  digitalWrite(LCD_BACKLIGHT, LOW); 
  lcd.clear();
  Serial.println("\n[休眠] 閒置超過10秒，進入省電休眠。");
}

void updateLCD() {
  if (isAsleep || isAlertMode) return; 

  lcd.setCursor(0, 0);
  if (isLightOn) {
    int percentage = map(currentBrightness, 0, 255, 0, 100);
    lcd.print("Light: ON, ");
    if (percentage < 100) lcd.print(" "); 
    if (percentage < 10)  lcd.print(" ");
    lcd.print(percentage);
    lcd.print("% ");
  } else {
    lcd.print("Light: OFF      "); 
  }

  lcd.setCursor(0, 1);
  if (isCurtainOpen) {
    lcd.print("Window: OPENED ");
  } else {
    lcd.print("Window: CLOSED ");
  }
}

// ==========================================
// 致動器控制副程式
// ==========================================

void toggleLight() {
  isLightOn = !isLightOn;
  if (isLightOn) {
    currentBrightness = 255;
    setRGB(currentBrightness, currentBrightness, currentBrightness);
  } else {
    setRGB(0, 0, 0);
  }
  updateLCD(); 
  shortBeep();
}

void toggleCurtain() {
  isCurtainOpen = !isCurtainOpen;
  if (isCurtainOpen) {
    myServo.write(25);
  } else {
    myServo.write(95);
  }
  updateLCD(); 
  longBeep(1);
}

void setRGB(uint8_t r, uint8_t g, uint8_t b) {
  analogWrite(LED_R, r);
  analogWrite(LED_G, g);
  analogWrite(LED_B, b);
}

void shortBeep() {
  digitalWrite(BUZZER_PIN, HIGH);
  delay(80); 
  digitalWrite(BUZZER_PIN, LOW);
}

void longBeep(int count) {
  for(int i = 0; i < count; i++) {
    digitalWrite(BUZZER_PIN, HIGH);
    delay(250);
    digitalWrite(BUZZER_PIN, LOW);
    if(count > 1) delay(100); 
  }
}