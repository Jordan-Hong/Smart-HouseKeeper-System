#include <Servo.h>
#include <SoftwareSerial.h>

#define BUTTON_PIN       2

#define FRONT_BUZZER     6
#define REAR_BUZZER      7

#define SERVO_PIN        9

#define BT_TX            10
#define BT_RX            11

#define FRONT_SENSOR     A0
#define REAR_SENSOR      A1

#define SENSOR_THRESHOLD 500

SoftwareSerial BT(BT_TX, BT_RX);
Servo doorServo;
bool doorOpened = false;

int counter = 0;

// --------------------
// 狀態
// --------------------

bool waitingRecognition = false;
bool waitingSafetyCheck = false;
bool recognitionFailed = false;

unsigned long recognitionStart = 0;
const unsigned long RECOGNITION_TIMEOUT = 10000;

// buzzer

bool frontBuzzing = false;
bool rearBuzzing = false;

unsigned long frontBuzzStart = 0;
unsigned long rearBuzzStart = 0;

const unsigned long BUZZ_TIME = 1000;

// 方向判斷

enum DirectionState
{
  IDLE,
  FRONT_FIRST,
  REAR_FIRST
};

DirectionState directionState = IDLE;

unsigned long directionTimer = 0;
const unsigned long DIRECTION_TIMEOUT = 60000;

// 感測器防重複

bool frontReady = true;
bool rearReady = true;

unsigned long lastFrontTrigger = 0;
unsigned long lastRearTrigger = 0;

const unsigned long SENSOR_COOLDOWN = 250;

const int RELEASE_LEVEL = 200;
// --------------------

void setup()
{
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  pinMode(FRONT_BUZZER, OUTPUT);
  pinMode(REAR_BUZZER, OUTPUT);

  digitalWrite(FRONT_BUZZER, LOW);
  digitalWrite(REAR_BUZZER, LOW);

  Serial.begin(9600);

  BT.begin(38400);

  doorServo.attach(SERVO_PIN);
  doorServo.write(0);

  Serial.println("System Ready");
}

void loop()
{
  handleButton();

  handleSensors();

  handlePC();

  handleBluetooth();

  handleBuzzer();

  handleTimeout();
}

//======================================================
// Button
//======================================================

void handleButton()
{
  static bool lastState = HIGH;

  bool current = digitalRead(BUTTON_PIN);

  if(lastState == HIGH && current == LOW)
  {
    startRearBuzzer();
  }

  lastState = current;
}

//======================================================
// Sensors
//======================================================

void handleSensors()
{
  int frontValue = analogRead(FRONT_SENSOR);
  int rearValue  = analogRead(REAR_SENSOR);

  bool frontTriggered = frontValue > SENSOR_THRESHOLD;
  bool rearTriggered  = rearValue > SENSOR_THRESHOLD;

  //----------------------------------
  // Front
  //----------------------------------

  if(frontTriggered &&
     frontReady &&
     millis() - lastFrontTrigger > SENSOR_COOLDOWN)
  {
    frontReady = false;

    lastFrontTrigger = millis();

    onFrontTriggered();

    Serial.print("frontValue=");
    Serial.println(frontValue);
  }

  if(frontValue < RELEASE_LEVEL)
  {
    frontReady = true;
    recognitionFailed = false;
  }

  //----------------------------------
  // Rear
  //----------------------------------

  if(rearTriggered &&
     rearReady &&
     millis() - lastRearTrigger > SENSOR_COOLDOWN)
  {
    rearReady = false;

    lastRearTrigger = millis();

    onRearTriggered();

    Serial.print("rearValue=");
    Serial.println(rearValue);
  }

  if(rearValue < RELEASE_LEVEL)
  {
    rearReady = true;
  }

  //----------------------------------
  // Direction timeout
  //----------------------------------

  if(directionState != IDLE)
  {
    if(millis() - directionTimer > DIRECTION_TIMEOUT)
    {
      directionState = IDLE;
    }
  }
}

//======================================================
// Front Sensor
//======================================================

void onFrontTriggered()
{
  // 離開流程
  if(directionState == REAR_FIRST)
  {
    closeDoor();

    if(counter > 0)
    {
      counter--;

      Serial.print("counter=");
      Serial.println(counter);

      if(counter == 0)
      {
        BT.write("E");

        waitingSafetyCheck = true;
      }
    }

    directionState = IDLE;

    return;
  }

  // 辨識失敗後尚未離開感測區
  if(recognitionFailed)
    return;

  // 已經在等待辨識結果
  if(waitingRecognition)
    return;

  // 進入流程
  Serial.println("recognition_request");

  waitingRecognition = true;

  recognitionStart = millis();
}

//======================================================
// Rear Sensor
//======================================================

void onRearTriggered()
{
//  ("rear_sensor");

  if(directionState == FRONT_FIRST)
  {
    closeDoor();

    counter++;

    Serial.print("counter=");
    Serial.println(counter);

    directionState = IDLE;

    return;
  }

  if(directionState == IDLE)
  {
    openDoor();

    directionState = REAR_FIRST;

    directionTimer = millis();
  }
}

//======================================================
// PC Serial
//======================================================

void handlePC()
{
  if(!Serial.available())
    return;

  String msg = Serial.readStringUntil('\n');

  msg.trim();

  if(waitingRecognition)
  {
    if(msg == "True")
    {
        openDoor();
    
        directionState = FRONT_FIRST;
    
        directionTimer = millis();
    
        waitingRecognition = false;
    }

    else if(msg == "False")
    {
      startFrontBuzzer();

      waitingRecognition = false;
    }
  }
}

//======================================================
// Bluetooth
//======================================================

void handleBluetooth()
{
    if(!BT.available())
        return;
    Serial.print("BT:");
    delay(100);

    String msg = "";

    while(BT.available())
    {
        msg += (char)BT.read();
    }

    msg.trim();
    Serial.println(msg);
    if(msg == "Q")
    {
        //String reply = "RES_COUNTER:" + String(counter);
        if(counter == 0)
        {
          BT.write("0");
          startFrontBuzzer();
          startRearBuzzer();
        }else{
          BT.write("1");
        }
    }

    if(waitingSafetyCheck)
    {
        if(msg == "A")
        {
            Serial.println("greeting=A");
            waitingSafetyCheck = false;
        }

        else if(msg == "B")
        {
            Serial.println("greeting=B");
            waitingSafetyCheck = false;
        }
    }
}

//======================================================
// Servo
//======================================================

void openDoor()
{
  doorServo.write(45);
  Serial.println("OOOOOOO");
  doorOpened = true;
}

void closeDoor()
{
  doorServo.write(0);
  Serial.println("XXXXXXX");
  doorOpened = false;
}
//======================================================
// Buzzer
//======================================================

void startFrontBuzzer()
{
  tone(FRONT_BUZZER, 50);

  frontBuzzing = true;

  frontBuzzStart = millis();
}

void startRearBuzzer()
{
  tone(REAR_BUZZER, 25);
  
  rearBuzzing = true;

  rearBuzzStart = millis();
}

void handleBuzzer()
{
  unsigned long now = millis();

  if(frontBuzzing)
  {
    if(now - frontBuzzStart > BUZZ_TIME)
    {
      noTone(FRONT_BUZZER);

      frontBuzzing = false;
    }
  }

  if(rearBuzzing)
  {
    if(now - rearBuzzStart > BUZZ_TIME)
    {
      noTone(REAR_BUZZER);

      rearBuzzing = false;
    }
  }
}

//======================================================
// Timeout
//======================================================

void handleTimeout()
{
  if(waitingRecognition)
  {
    if(millis() - recognitionStart > RECOGNITION_TIMEOUT)
    {
      waitingRecognition = false;

      recognitionFailed = true;

      startFrontBuzzer();
    }
  }
}
