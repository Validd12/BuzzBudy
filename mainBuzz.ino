#include <Wire.h>
#include "RTClib.h"
#include <LiquidCrystal.h>
#include <EEPROM.h>
#include <dht_nonblocking.h>
#include <Servo.h>

Servo alarmServo;
boolean movementDetected = false;
boolean flameDetected = false; // Add this line
const int shakePin = A0;
const int flamePin = A1;
const int servoPin = 11;
#define DHT_SENSOR_TYPE DHT_TYPE_11
LiquidCrystal lcd(2,3,4,5,6,7);
int tempC;
double tempK;
int tempReading;
unsigned long starttime;
float temperature;
float humidity;
byte clocksymbol[8] = {
  0b00000,
  0b00100,
  0b01110,
  0b01110,
  0b11111,
  0b00000,
  0b00100,
  0b00000,
};

byte temp[8] = {
  0b10000,
  0b00111,
  0b01000,
  0b01000,
  0b01000,
  0b01000,
  0b00111,
  0b00000,
};

static const int DHT_SENSOR_PIN = 10;
const int buzzer = 9;
const int isbacklight = 8;
DHT_nonblocking dht_sensor(DHT_SENSOR_PIN, DHT_SENSOR_TYPE);

RTC_DS1307 rtc;
char daysOfTheWeek[7][12] = {"Sunday","Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
const int btSet = A5;
const int btAdj = A4;
const int btAlarm = A3;

long interval = 300;
int melody[] = {600, 800, 1000,1200};

int DD,MM,YY,H,M,S,set_state, adjust_state, alarm_state, AH, AM;
int shakeTimes=0;
int i =0;
int btnCount = 0;
String sDD;
String sMM;
String sYY;
String sH;
String sM;
String sS;
String aH="12";
String aM="00";
String alarm = "     ";
long previousMillis = 0;

boolean setupScreen = false;
boolean alarmON=false;
boolean turnItOn = false;
boolean backlightON=true;
unsigned long movementStartTime = 0;

void setup() {
  pinMode(shakePin, INPUT); // Pin pentru senzorul de șoc
  pinMode(flamePin, INPUT); // Pin pentru senzorul IR de flacără
  rtc.begin();
  lcd.begin(16,2);
  pinMode(btSet,INPUT_PULLUP);
  pinMode(btAdj,INPUT_PULLUP);
  pinMode(btAlarm, INPUT_PULLUP);
  pinMode(buzzer, OUTPUT);

  Serial.begin(9600);
  lcd.createChar(1, clocksymbol);
  lcd.createChar(2, temp);

  digitalWrite(isbacklight,HIGH);
  alarmServo.attach(servoPin);

  if (! rtc.isrunning()) {
    rtc.adjust(DateTime(2018, 01, 01, 00, 00, 0));
  }
  delay(100);

  AH=EEPROM.read(0);
  AM=EEPROM.read(1);
  if (AH>23){
    AH=0;
  }
  if (AM>59){
    AM=0;
  }
}

void loop() {
  checkShakeSensor();
  checkFlameSensor();
  readBtns();
  getTimeDate();
  if (!setupScreen){
    lcdPrint();
    if (alarmON){
      callAlarm();
    }
    else{
      alarmServo.write(45);
      lcd.setCursor(10, 0);
      lcd.write(" ");
    }
  }
  else{
    timeSetup();
  }

  if( measure_environment( &temperature, &humidity ) == true )
  {
     tempC =temperature;
  }

  if (movementDetected) {
    unsigned long currentMillis = millis();
    if (currentMillis - movementStartTime >= 15000) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Dispozitivul a fost");
      lcd.setCursor(0, 1);
      lcd.print("miscat");
      tone(buzzer, 1000, 500);
      delay(2000);
      movementDetected = false;
    }
  }
}

static bool measure_environment( float *temperature, float *humidity )
{
  static unsigned long measurement_timestamp = millis();

  if( millis() - measurement_timestamp > 3000ul )
  {
    if( dht_sensor.measure( temperature, humidity ) == true )
    {
      measurement_timestamp = millis();
      return( true );
    }
  }
  return( false );
}

void checkShakeSensor() {
  int shakeState = analogRead(shakePin);
  if (shakeState > 500) { // Ajustează pragul dacă este necesar
    Serial.println("Mișcare detectată!");
    movementDetected = true;
    callEmergencyAlarm();
  } else {
    movementDetected = false;
  }
}

void checkFlameSensor() {
  int flameState = analogRead(flamePin);
  if (flameState < 300) { // Prag scăzut indică flacără
    Serial.println("Flacără detectată!");
    flameDetected = true;
    callEmergencyAlarm();
  } else {
    flameDetected = false;
  }
}

void callEmergencyAlarm() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Pericol Detectat!");
  lcd.setCursor(0, 1);
  lcd.print("ALARM ACTIVAT!");

  // Activează buzzer-ul și servo-ul în caz de urgență
  for (int j = 0; j < 5; j++) {
    alarmServo.write(180);
    tone(buzzer, 1200, 500);
    delay(500);
    alarmServo.write(45);
    delay(500);
  }
}


void readBtns(){
  set_state = digitalRead(btSet);
  adjust_state = digitalRead(btAdj);
  alarm_state = digitalRead(btAlarm);
  if(!setupScreen){
    if (alarm_state==LOW){
      if (alarmON){
        alarm="     ";
        alarmON=false;
      }
      else{
        alarmON=true;
      }
      delay(500);
    }

  if (adjust_state==LOW){
      if (backlightON==true){
        backlightON=false;
        digitalWrite(isbacklight,LOW);
      }
      else{
        backlightON=true;
        digitalWrite(isbacklight,HIGH);
      }
      delay(500);
    }
  }
  if (set_state==LOW){
    if(btnCount<7){
      btnCount++;
      setupScreen = true;
      if(btnCount==1){
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("----SETEAZA----");
        lcd.setCursor(0,1);
        lcd.print("-TIMPUL SI DATA-");
        delay(2000);
        lcd.clear();
      }
    }
    else{
      lcd.clear();
      rtc.adjust(DateTime(YY, MM, DD, H, M, 0));
      EEPROM.write(0, AH); 
      EEPROM.write(1, AM); 
      lcd.print("SE SALVEAZA....");
      delay(2000);
      lcd.clear();
      setupScreen = false;
      btnCount=0;
    }
    delay(500);
  }
}

void getTimeDate(){
  if (!setupScreen){
    DateTime now = rtc.now();
    DD = now.day();
    MM = now.month();
    YY = now.year();
    H = now.hour();
    M = now.minute();
    S = now.second();
  }
  if (DD<10){ sDD = '0' + String(DD); } else { sDD = DD; }
  if (MM<10){ sMM = '0' + String(MM); } else { sMM = MM; }
  sYY=YY-2000;
  if (H<10){ sH = '0' + String(H); } else { sH = H; }
  if (M<10){ sM = '0' + String(M); } else { sM = M; }
  if (S<10){ sS = '0' + String(S); } else { sS = S; }
  if (AH<10){ aH = '0' + String(AH); } else { aH = AH; }
  if (AM<10){ aM = '0' + String(AM); }  else { aM = AM; }
}

void lcdPrint(){
  String line1 = sH+":"+sM+":"+sS+" |";
  lcd.setCursor(0,0); 
   lcd.print(line1);
   if (alarmON){
     lcd.setCursor(10, 0);
     lcd.write(1);
   }
  String line2 = aH+":"+aM;
   lcd.setCursor(11, 0);
   lcd.print(line2);

  String line3 = sDD+"-"+sMM+"-"+sYY +" | " +22 ;
  lcd.setCursor(0,1); 
  lcd.print(line3);  
  if (setupScreen){
    lcd.setCursor(13, 1);
    lcd.print("");  
  }
  else{
    lcd.setCursor(13, 1);
    lcd.write(2);
  }
}

void timeSetup(){
  int up_state = adjust_state;
  int down_state = alarm_state;
  if(btnCount<=5){
    if (btnCount==1){
      lcd.setCursor(4,0);
      lcd.print(">"); 
      if (up_state == LOW){ 
        if (H<23){
          H++;
        }
        else {
          H=0;
        }
        delay(350);
      }
      if (down_state == LOW){
        if (H>0){
          H--;
        }
        else {
          H=23;
        }
        delay(350);
      }
    }
    else if (btnCount==2){   
      lcd.setCursor(4,0);
      lcd.print(" ");
      lcd.setCursor(9,0);
      lcd.print(">");
      if (up_state == LOW){
        if (M<59){
          M++;
        }
        else {
          M=0;
        }
        delay(350);
      }
      if (down_state == LOW){
        if (M>0){
          M--;
        }
        else {
          M=59;
        }
        delay(350);
      }
    }
    else if (btnCount==3){   
      lcd.setCursor(9,0);
      lcd.print(" ");
      lcd.setCursor(0,1);
      lcd.print(">");
      if (up_state == LOW){
        if (DD<31){
          DD++;
        }
        else {
          DD=1;
        }
        delay(350);
      }
      if (down_state == LOW){
        if (DD>1){
          DD--;
        }
        else {
          DD=31;
        }
        delay(350);
      }
    }
    else if (btnCount==4){
      lcd.setCursor(0,1);
      lcd.print(" ");
      lcd.setCursor(5,1);
      lcd.print(">");
      if (up_state == LOW){
        if (MM<12){
          MM++;
        }
        else {
          MM=1;
        }
        delay(350);
      }
      if (down_state == LOW){
        if (MM>1){
          MM--;
        }
        else {
          MM=12;
        }
        delay(350);
      }
    }
    else if (btnCount==5){ 
      lcd.setCursor(5,1);
      lcd.print(" ");
      lcd.setCursor(10,1);
      lcd.print(">");
      if (up_state == LOW){
        if (YY<2099){
          YY++;
        }
        else {
          YY=2000;
        }
        delay(350);
      }
      if (down_state == LOW){
        if (YY>2000){
          YY--;
        }
        else {
          YY=2099;
        }
        delay(350);
      }
    }
  }
  else{
    if (up_state==LOW){
      AH++;
      delay(350);
    }
    if (down_state == LOW){
      AM--;
      delay(350);
    }
  }
}

void callAlarm(){
  if (AH==H && AM==M && alarmON){
    alarmServo.write(180);
    tone(buzzer, melody[i], interval);
    i++;//add 
    if (i > 3){
      i = 0;
    }
  }
}
ChatGPT said:
Great project 👍 You already have a very advanced smart-home / alarm system.
Below I’ll give you everything needed to add Bluetooth control, step by step:

1️⃣ What we will add
Hardware
Bluetooth module HC-05 or HC-06

A few jumper wires

Software
Bluetooth serial communication in Arduino

Command protocol (simple text commands)

Android mobile app (MIT App Inventor – easiest)

Optional: Serial Bluetooth Terminal (for quick testing)

2️⃣ Bluetooth hardware connection (HC-05 / HC-06)
HC-05 pins → Arduino UNO
HC-05	Arduino
VCC	5V
GND	GND
TXD	D12
RXD	D13 (via 1k–2k resistor recommended)
⚠️ HC-05 RX must be protected (Arduino TX is 5V)

3️⃣ Bluetooth logic – what the phone can control
We will support commands like:

ALARM_ON
ALARM_OFF
BACKLIGHT_ON
BACKLIGHT_OFF
SERVO_OPEN
SERVO_CLOSE
STATUS
4️⃣ Add Bluetooth code to your sketch
🔹 Add library & pins (TOP of code)
#include <SoftwareSerial.h>

#define BT_RX 12
#define BT_TX 13

SoftwareSerial bt(BT_RX, BT_TX);
String btCommand = "";
🔹 Start Bluetooth in setup()
bt.begin(9600);
bt.println("Smart Home Ready");
🔹 Add Bluetooth handler in loop()
Add this inside loop(), near the top:

readBluetooth();
🔹 Bluetooth command function (ADD THIS)
void readBluetooth() {
  while (bt.available()) {
    char c = bt.read();
    if (c == '\n') {
      processBTCommand(btCommand);
      btCommand = "";
    } else {
      btCommand += c;
    }
  }
}
🔹 Command processor (ADD THIS)
void processBTCommand(String cmd) {
  cmd.trim();
  Serial.println("BT: " + cmd);

  if (cmd == "ALARM_ON") {
    alarmON = true;
    bt.println("Alarm ON");
  }
  else if (cmd == "ALARM_OFF") {
    alarmON = false;
    bt.println("Alarm OFF");
  }
  else if (cmd == "BACKLIGHT_ON") {
    digitalWrite(isbacklight, HIGH);
    backlightON = true;
    bt.println("Backlight ON");
  }
  else if (cmd == "BACKLIGHT_OFF") {
    digitalWrite(isbacklight, LOW);
    backlightON = false;
    bt.println("Backlight OFF");
  }
  else if (cmd == "SERVO_OPEN") {
    alarmServo.write(180);
    bt.println("Servo OPEN");
  }
  else if (cmd == "SERVO_CLOSE") {
    alarmServo.write(45);
    bt.println("Servo CLOSE");
  }
  else if (cmd == "STATUS") {
    bt.print("TEMP:");
    bt.print(tempC);
    bt.print(" ALARM:");
    bt.println(alarmON ? "ON" : "OFF");
  }
  else {
    bt.println("UNKNOWN COMMAND");
  }
}
5️⃣ Android App (MIT App Inventor)
🔹 Components needed
Visible

6 Buttons

Alarm ON

Alarm OFF

Light ON

Light OFF

Servo Open

Servo Close

1 Label (status)

Non-visible

BluetoothClient

ListPicker (Bluetooth devices)

🔹 Button → Command mapping
Button	Text sent
Alarm ON	ALARM_ON\n
Alarm OFF	ALARM_OFF\n
Light ON	BACKLIGHT_ON\n
Light OFF	BACKLIGHT_OFF\n
Servo Open	SERVO_OPEN\n
Servo Close	SERVO_CLOSE\n
🔹 MIT App Inventor Blocks (logic)
Connect

ListPicker → BluetoothClient.Connect
Send command

BluetoothClient.SendText("ALARM_ON\n")
6️⃣ Quick testing (PC / Android)
Before building the app:

Install Serial Bluetooth Terminal

Pair HC-05 (PIN: 1234 or 0000)

Send:

ALARM_ON
You should see:

Servo move

Alarm logic active

LCD update

7️⃣ Final system architecture
[ Phone App ]
      ↓ Bluetooth
[ HC-05 Module ]
      ↓ Serial
[ Arduino ]
      ↓
Sensors + LCD + Servo + Buzzer
8️⃣ Optional upgrades (next level 🔥)
Bluetooth password authentication

Emergency notification to phone

Replace HC-05 with ESP32 (WiFi + Bluetooth)

Mobile app with live temperature chart

Voice commands

9️⃣ If you want
I can:

📱 Build the MIT App Inventor file for you

🤖 Convert this into ESP32 + WiFi smart home

🧠 Add AI-based detection logic

📦 Draw a full wiring diagram

Just tell me 👍

You said:
yes I want all the files complete and with name please
ChatGPT said:
Perfect 👍
Below you have ALL FILES COMPLETE, clearly separated, with exact file names, ready to upload/use.
Nothing missing.

📁 FILE 1 — Arduino Code
File name: SmartHome_Alarm_BT.ino

👉 Upload this directly to Arduino UNO

#include <Wire.h>
#include "RTClib.h"
#include <LiquidCrystal.h>
#include <EEPROM.h>
#include <dht_nonblocking.h>
#include <Servo.h>
#include <SoftwareSerial.h>

/* ================= BLUETOOTH ================= */
#define BT_RX 12
#define BT_TX 13
SoftwareSerial bt(BT_RX, BT_TX);
String btCommand = "";

/* ================= HARDWARE ================= */
Servo alarmServo;
RTC_DS1307 rtc;
LiquidCrystal lcd(2,3,4,5,6,7);

#define DHT_SENSOR_TYPE DHT_TYPE_11
static const int DHT_SENSOR_PIN = 10;
DHT_nonblocking dht_sensor(DHT_SENSOR_PIN, DHT_SENSOR_TYPE);

const int shakePin = A0;
const int flamePin = A1;
const int servoPin = 11;
const int buzzer = 9;
const int isbacklight = 8;

const int btSet = A5;
const int btAdj = A4;
const int btAlarm = A3;

/* ================= VARIABLES ================= */
boolean movementDetected = false;
boolean flameDetected = false;
boolean alarmON = false;
boolean backlightON = true;
boolean setupScreen = false;

int AH = 12, AM = 0;
int DD
You said:
i dont want comments
ChatGPT said:
Got it! Here is the full Arduino code without any comments, clean and ready to use:

File: SmartHome_Alarm_BT.ino
#include <Wire.h>
#include "RTClib.h"
#include <LiquidCrystal.h>
#include <EEPROM.h>
#include <dht_nonblocking.h>
#include <Servo.h>
#include <SoftwareSerial.h>

#define BT_RX 12
#define BT_TX 13
SoftwareSerial bt(BT_RX, BT_TX);
String btCommand = "";

Servo alarmServo;
RTC_DS1307 rtc;
LiquidCrystal lcd(2,3,4,5,6,7);

#define DHT_SENSOR_TYPE DHT_TYPE_11
static const int DHT_SENSOR_PIN = 10;
DHT_nonblocking dht_sensor(DHT_SENSOR_PIN, DHT_SENSOR_TYPE);

const int shakePin = A0;
const int flamePin = A1;
const int servoPin = 11;
const int buzzer = 9;
const int isbacklight = 8;

const int btSet = A5;
const int btAdj = A4;
const int btAlarm = A3;

boolean movementDetected = false;
boolean flameDetected = false;
boolean alarmON = false;
boolean backlightON = true;
boolean setupScreen = false;

int AH = 12, AM = 0;
int DD, MM, YY, H, M, S;
int set_state, adjust_state, alarm_state;
int i = 0;
int btnCount = 0;

byte clocksymbol[8] = {
  0b00000,
  0b00100,
  0b01110,
  0b01110,
  0b11111,
  0b00000,
  0b00100,
  0b00000,
};

byte temp[8] = {
  0b10000,
  0b00111,
  0b01000,
  0b01000,
  0b01000,
  0b01000,
  0b00111,
  0b00000,
};

int tempC;
float temperature;
float humidity;

String sDD, sMM, sYY, sH, sM, sS, aH, aM;

unsigned long movementStartTime = 0;

void setup() {
  pinMode(shakePin, INPUT);
  pinMode(flamePin, INPUT);
  pinMode(btSet, INPUT_PULLUP);
  pinMode(btAdj, INPUT_PULLUP);
  pinMode(btAlarm, INPUT_PULLUP);
  pinMode(buzzer, OUTPUT);

  rtc.begin();
  lcd.begin(16, 2);
  digitalWrite(isbacklight, HIGH);
  pinMode(isbacklight, OUTPUT);

  alarmServo.attach(servoPin);

  bt.begin(9600);
  bt.println("Smart Home Ready");

  lcd.createChar(1, clocksymbol);
  lcd.createChar(2, temp);

  if (!rtc.isrunning()) {
    rtc.adjust(DateTime(2018, 1, 1, 0, 0, 0));
  }

  AH = EEPROM.read(0);
  AM = EEPROM.read(1);
  if (AH > 23) AH = 0;
  if (AM > 59) AM = 0;
}

void loop() {
  readBluetooth();
  checkShakeSensor();
  checkFlameSensor();
  readBtns();
  getTimeDate();

  if (!setupScreen) {
    lcdPrint();
    if (alarmON) callAlarm();
    else {
      alarmServo.write(45);
      lcd.setCursor(10, 0);
      lcd.write(" ");
    }
  } else {
    timeSetup();
  }

  if (measure_environment(&temperature, &humidity)) {
    tempC = temperature;
  }

  if (movementDetected) {
    if (millis() - movementStartTime >= 15000) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Dispozitivul a fost");
      lcd.setCursor(0, 1);
      lcd.print("miscat");
      tone(buzzer, 1000, 500);
      delay(2000);
      movementDetected = false;
    }
  }
}

void readBluetooth() {
  while (bt.available()) {
    char c = bt.read();
    if (c == '\n') {
      processBTCommand(btCommand);
      btCommand = "";
    } else {
      btCommand += c;
    }
  }
}

void processBTCommand(String cmd) {
  cmd.trim();

  if (cmd == "ALARM_ON") {
    alarmON = true;
    bt.println("Alarm ON");
  } else if (cmd == "ALARM_OFF") {
    alarmON = false;
    bt.println("Alarm OFF");
  } else if (cmd == "BACKLIGHT_ON") {
    digitalWrite(isbacklight, HIGH);
    backlightON = true;
    bt.println("Backlight ON");
  } else if (cmd == "BACKLIGHT_OFF") {
    digitalWrite(isbacklight, LOW);
    backlightON = false;
    bt.println("Backlight OFF");
  } else if (cmd == "SERVO_OPEN") {
    alarmServo.write(180);
    bt.println("Servo OPEN");
  } else if (cmd == "SERVO_CLOSE") {
    alarmServo.write(45);
    bt.println("Servo CLOSE");
  } else if (cmd == "STATUS") {
    bt.print("TEMP:");
    bt.print(tempC);
    bt.print(" ALARM:");
    bt.println(alarmON ? "ON" : "OFF");
  } else {
    bt.println("UNKNOWN COMMAND");
  }
}

static bool measure_environment(float *temperature, float *humidity) {
  static unsigned long measurement_timestamp = millis();

  if (millis() - measurement_timestamp > 3000ul) {
    if (dht_sensor.measure(temperature, humidity) == true) {
      measurement_timestamp = millis();
      return true;
    }
  }
  return false;
}

void checkShakeSensor() {
  int shakeState = analogRead(shakePin);
  if (shakeState > 500) {
    movementDetected = true;
    movementStartTime = millis();
    callEmergencyAlarm();
  } else {
    movementDetected = false;
  }
}

void checkFlameSensor() {
  int flameState = analogRead(flamePin);
  if (flameState < 300) {
    flameDetected = true;
    callEmergencyAlarm();
  } else {
    flameDetected = false;
  }
}

void callEmergencyAlarm() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Pericol Detectat!");
  lcd.setCursor(0, 1);
  lcd.print("ALARM ACTIVAT!");

  for (int j = 0; j < 5; j++) {
    alarmServo.write(180);
    tone(buzzer, 1200, 500);
    delay(500);
    alarmServo.write(45);
    delay(500);
  }
}

void readBtns() {
  set_state = digitalRead(btSet);
  adjust_state = digitalRead(btAdj);
  alarm_state = digitalRead(btAlarm);

  if (!setupScreen) {
    if (alarm_state == LOW) {
      alarmON = !alarmON;
      delay(500);
    }
    if (adjust_state == LOW) {
      backlightON = !backlightON;
      digitalWrite(isbacklight, backlightON ? HIGH : LOW);
      delay(500);
    }
  }

  if (set_state == LOW) {
    if (btnCount < 7) {
      btnCount++;
      setupScreen = true;
      if (btnCount == 1) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("----SETEAZA----");
        lcd.setCursor(0, 1);
        lcd.print("-TIMPUL SI DATA-");
        delay(2000);
        lcd.clear();
      }
    } else {
      lcd.clear();
      rtc.adjust(DateTime(YY, MM, DD, H, M, 0));
      EEPROM.write(0, AH);
      EEPROM.write(1, AM);
      lcd.print("SE SALVEAZA....");
      delay(2000);
      lcd.clear();
      setupScreen = false;
      btnCount = 0;
    }
    delay(500);
  }
}

void getTimeDate() {
  if (!setupScreen) {
    DateTime now = rtc.now();
    DD = now.day();
    MM = now.month();
    YY = now.year();
    H = now.hour();
    M = now.minute();
    S = now.second();
  }
  sDD = (DD < 10 ? "0" : "") + String(DD);
  sMM = (MM < 10 ? "0" : "") + String(MM);
  sYY = String(YY - 2000);
  sH = (H < 10 ? "0" : "") + String(H);
  sM = (M < 10 ? "0" : "") + String(M);
  sS = (S < 10 ? "0" : "") + String(S);
  aH = (AH < 10 ? "0" : "") + String(AH);
  aM = (AM < 10 ? "0" : "") + String(AM);
}

void lcdPrint() {
  lcd.setCursor(0, 0);
  lcd.print(sH + ":" + sM + ":" + sS + " |");
  if (alarmON) {
    lcd.setCursor(10, 0);
    lcd.write(1);
  }
  lcd.setCursor(11, 0);
  lcd.print(aH + ":" + aM);
  lcd.setCursor(0, 1);
  lcd.print(sDD + "-" + sMM + "-" + sYY + " | " + 22);
  if (setupScreen) {
    lcd.setCursor(13, 1);
    lcd.print("");
  } else {
    lcd.setCursor(13, 1);
    lcd.write(2);
  }
}

void timeSetup() {
  int up_state = adjust_state;
  int down_state = alarm_state;
  if (btnCount <= 5) {
    if (btnCount == 1) {
      lcd.setCursor(4, 0);
      lcd.print(">");
      if (up_state == LOW) {
        H = (H < 23) ? H + 1 : 0;
        delay(350);
      }
      if (down_state == LOW) {
        H = (H > 0) ? H - 1 : 23;
        delay(350);
      }
    } else if (btnCount == 2) {
      lcd.setCursor(4, 0);
      lcd.print(" ");
      lcd.setCursor(9, 0);
      lcd.print(">");
      if (up_state == LOW) {
        M = (M < 59) ? M + 1 : 0;
        delay(350);
      }
      if (down_state == LOW) {
        M = (M > 0) ? M - 1 : 59;
        delay(350);
      }
    } else if (btnCount == 3) {
      lcd.setCursor(9, 0);
      lcd.print(" ");
      lcd.setCursor(0, 1);
      lcd.print(">");
      if (up_state == LOW) {
        DD = (DD < 31) ? DD + 1 : 1;
        delay(350);
      }
      if (down_state == LOW) {
        DD = (DD > 1) ? DD - 1 : 31;
        delay(350);
      }
    } else if (btnCount == 4) {
      lcd.setCursor(0, 1);
      lcd.print(" ");
      lcd.setCursor(5, 1);
      lcd.print(">");
      if (up_state == LOW) {
        MM = (MM < 12) ? MM + 1 : 1;
        delay(350);
      }
      if (down_state == LOW) {
        MM = (MM > 1) ? MM - 1 : 12;
        delay(350);
      }
    } else if (btnCount == 5) {
      lcd.setCursor(5, 1);
      lcd.print(" ");
      lcd.setCursor(10, 1);
      lcd.print(">");
      if (up_state == LOW) {
        YY = (YY < 2099) ? YY + 1 : 2000;
        delay(350);
      }
      if (down_state == LOW) {
        YY = (YY > 2000) ? YY - 1 : 2099;
        delay(350);
      }
    }
  } else {
    if (up_state == LOW) {
      AH++;
      delay(350);
    }
    if (down_state == LOW) {
      AM--;
      delay(350);
    }
  }
}

void callAlarm() {
  if (AH == H && AM == M && alarmON) {
    alarmServo.write(180);
    tone(buzzer, 600 + 200 * (i % 4), 300);
    i++;
    if (i > 3) i = 0;
  }
}
