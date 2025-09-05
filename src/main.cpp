/*
  Connections:
  NodeMCU    -> Matrix
  MOSI-D7-GPIO13  -> DIN
  CLK-D5-GPIO14   -> CK
  GPIO0-D3        -> CS
  SDA (D2)        -> DS3231 SDA
  SCL (D1)        -> DS3231 SCL
  D4              -> Alarm output (buzzer/relay)
*/

#include <ESP8266WiFi.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <RTClib.h>

#define SDA_PIN 4 // D2
#define SCL_PIN 5 // D1

#define ALARM_PIN 12 // D6
#define LED_WIFI 0   // D3

// Pins
#define STATUS_LED 14 // Status LED on D5

// define ALERT_PIN TX
// #define ALERT_PIN 14 // St

const char *ssid = "HuynhHuu";
const char *password = "@0898524927";

WiFiClient client;
RTC_DS3231 rtc;
int TIMEZONE = 7; // UTC+7

// bool alertLog = false;
// bool lastButtonState = HIGH; 
// unsigned long lastDebounceTime = 0;
// unsigned long debounceDelay = 50; 

// System monitoring
unsigned long lastStatusCheck = 0;
const long STATUS_CHECK_INTERVAL = 15000;

void showTime(const char* txt, const DateTime &t) {
  Serial.print(txt);
  Serial.print(": ");
  Serial.print(t.year(), DEC);
  Serial.print('-');
  Serial.print(t.month(), DEC);
  Serial.print('-');
  Serial.print(t.day(), DEC);
  Serial.print(" ");
  Serial.print(t.hour(), DEC);
  Serial.print(':');
  Serial.print(t.minute(), DEC);
  Serial.print(':');
  Serial.print(t.second(), DEC);
  Serial.println();
}

void GetTimeFromGoogle(){
  WiFiClient client;
  
  const char* host = "www.google.com";
  const int httpPort = 80;

  if (!client.connect(host, httpPort)) {
    Serial.println("Connection to time server failed");
    rtc.adjust(DateTime(__DATE__, __TIME__));
    return;
  }

  client.print(String("GET / HTTP/1.1\r\n") + String("Host: www.google.com\r\n") + String("Connection: close\r\n\r\n"));

  int repeatCounter = 0;
  while (!client.available() && repeatCounter < 10){
    delay(100);
    repeatCounter++;
  }

  String line;
  while (client.connected() && client.available()){
    line = client.readStringUntil('\n');
    line.toUpperCase();
    if (line.startsWith("DATE: ")) {
      String dateStr = line.substring(6, 9);
      String dayStr = line.substring(11, 13);
      String monthStr = line.substring(14, 17);
      String yearStr = line.substring(18, 22);

      String timeStr = line.substring(23, 31);
      int h = timeStr.substring(0, 2).toInt();
      int m = timeStr.substring(3, 5).toInt();
      int s = timeStr.substring(6, 8).toInt();
      
      // Adjust for timezone (UTC+7)
      h = (h + TIMEZONE) % 24;

      // Convert month string to number
      int month = 0;
      if (monthStr == "JAN") month = 1;
      else if (monthStr == "FEB") month = 2;
      else if (monthStr == "MAR") month = 3;
      else if (monthStr == "APR") month = 4;
      else if (monthStr == "MAY") month = 5;
      else if (monthStr == "JUN") month = 6;
      else if (monthStr == "JUL") month = 7;
      else if (monthStr == "AUG") month = 8;
      else if (monthStr == "SEP") month = 9;
      else if (monthStr == "OCT") month = 10;
      else if (monthStr == "NOV") month = 11;
      else if (monthStr == "DEC") month = 12;

      int day = dayStr.toInt();
      int year = yearStr.toInt();

      // Set RTC time with full date
      DateTime now = rtc.now();
      int hNow = now.hour() / 10 * 10 + now.hour() % 10;
      int mNow = now.minute() / 10 * 10 + now.minute() % 10;
      if(hNow != h && mNow != m){
        Serial.println("Date found: " + line);
        Serial.print("RTC set to: ");
        Serial.print(year);
        Serial.print("-");
        Serial.print(month);
        Serial.print("-");
        Serial.print(day);
        Serial.print(" ");
        Serial.print(h);
        Serial.print(":");
        Serial.print(m);
        Serial.print(":");
        Serial.println(s);
        rtc.adjust(DateTime(year, month, day, h, m, s));
      }
      
      break;
    }
  }

  if (!client.connected() && !client.available()){
    rtc.adjust(DateTime(__DATE__, __TIME__));
  }

  client.stop();
}

struct Alarm{
  int alarmHour;
  int alarmMinute;
  int alarmSecond;
};

bool skipAlarm = false;
bool alarmTriggered = false;
const int TOTAL_ACTIVATIONS = 4;
const unsigned long ACTIVATION_DURATION = 10500;

const int size = 5;
Alarm alarmData[size] = {
  {6, 54, 0}, // First alarm
  {7, 1, 0},  // Second alarm
  {7, 15, 0}, // Third alarm
};


struct TimeData {
  int nHour;
  int nMinute;
  int nSecond;
};

TimeData GetTime() {
  DateTime dateTime = rtc.now();
  // showTime("Current time", dateTime);
  int nHour = dateTime.hour() / 10 * 10 + dateTime.hour() % 10;
  int nMinute = dateTime.minute() / 10 * 10 + dateTime.minute() % 10;
  int nSecond = dateTime.second() / 10 * 10 + dateTime.second() % 10;

  int dayOfTheWeek = dateTime.dayOfTheWeek();
  bool isSaturday = (dayOfTheWeek - 6) == 0;
  bool isSunday = (dayOfTheWeek - 0) == 0;
  // Serial.println("Day of the week: " + StringdayOfTheWeek) + "\n");
  // Serial.println("Saturday: " + String(isSaturday));
  // Serial.println("Sunday: " + String(isSunday));

  // Skip weekends (0 = Sunday, 6 = Saturday, )
  // Skip alarm on weekends
  if (isSaturday || isSunday) {
    // Serial.println("Weekend detected, skipping alarm");
    skipAlarm = true;
  }

  // Skip alarm at midnight
  if (nHour == 0 && nMinute == 0) {
    // Serial.println("Midnight detected, skipping alarm");
    skipAlarm = true;
  }

  return {
    nHour,
    nMinute,
    nSecond,
  };
}

void CheckAlarm() {
  int activeAlarm = 0;
  TimeData timeData = GetTime();

  int nHour = timeData.nHour;
  int nMinute = timeData.nMinute;
  int nSecond = timeData.nSecond;
  
  // Skip alarm on weekends
  if (skipAlarm) {
    // Serial.println("Alarm skipped due to weekend or midnight");
    alarmTriggered = false;
    skipAlarm = false;
    return;
  } 
  
  else {
    skipAlarm = false;
    for (int i = 0; i < size; i++){
      Alarm alarm = alarmData[i];
      if (nHour == alarm.alarmHour 
        && nMinute == alarm.alarmMinute 
        && nSecond == alarm.alarmSecond 
        && !alarmTriggered
      ){
        alarmTriggered = true;
        Serial.println("Alarm Triggered!");
        break;
      }else{
        alarmTriggered = false;
      }
    }

    if(alarmTriggered) {
      while (activeAlarm <= (round(60000 / ACTIVATION_DURATION))) {
        // Serial.println("Alarm Triggered! " + String(activeAlarm));
        if(activeAlarm < TOTAL_ACTIVATIONS){
          digitalWrite(ALARM_PIN, HIGH);
          delay(100);
          digitalWrite(ALARM_PIN, LOW);
        }
        delay(ACTIVATION_DURATION);
        activeAlarm++;
      }

      activeAlarm = 0;
      digitalWrite(ALARM_PIN, LOW);
      alarmTriggered = false;
    }
    else {
      digitalWrite(ALARM_PIN, LOW);
    }
  }
}

bool ConnectToWiFi() {
  const int maxRetries = 10;
  const int retryDelay = 3000;
  int retryCount = 0;
  bool connected = false;

  // If already connected, return true
  if (WiFi.status() == WL_CONNECTED) {
    return true;
  }

  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, password);

  while (retryCount < maxRetries && !connected) {
    if (WiFi.status() != WL_CONNECTED) {
      digitalWrite(LED_WIFI, !digitalRead(LED_WIFI));
      Serial.print(".");
      retryCount++;
      Serial.println(" Attempt " + String(retryCount) + " of " + String(maxRetries));
      delay(retryDelay);
    } else {
      connected = true;
    }
  }

  if (connected) {
    Serial.println("\nWiFi connected!");
    for (int i = 0; i < 3; i++) {
      digitalWrite(LED_WIFI, !digitalRead(LED_WIFI));
      delay(200);
    }
    digitalWrite(LED_WIFI, LOW);
    return true;
  } else {
    Serial.println("\nFailed to connect to WiFi");
    digitalWrite(LED_WIFI, LOW);
    return false;
  }
}

void AutoReconnectWiFi() {
  static unsigned long lastAttemptTime = 0;
  const unsigned long retryInterval = 30000;

  if (WiFi.status() != WL_CONNECTED && millis() - lastAttemptTime >= retryInterval) {
    Serial.println("Attempting WiFi reconnection...");
    lastAttemptTime = millis();
    ConnectToWiFi();
  }else{
    digitalWrite(LED_WIFI, LOW);
  }
}

void BlinkError(int count) {
  for(int i=0; i<count; i++) {
    digitalWrite(STATUS_LED, HIGH);
    delay(100);
    digitalWrite(STATUS_LED, LOW);
    delay(100);
  }
  delay(1000);
}

void CheckSystem(){
  unsigned long currentMillis = millis();

  // System status monitoring
  if(currentMillis - lastStatusCheck > STATUS_CHECK_INTERVAL) {
    lastStatusCheck = currentMillis;

    if(ESP.getFreeHeap() < 10000) {
      Serial.println("WARNING: Low memory");
      BlinkError(2);
    }

    if (!rtc.begin()){
      Serial.println("RTC FAILURE");
      BlinkError(3);
    }
  
    digitalWrite(STATUS_LED, HIGH);
    delay(1000);
    digitalWrite(STATUS_LED, LOW);
  }
}

void loop(){
  CheckAlarm(); // Check for alarms
  CheckSystem(); // Check system status
  AutoReconnectWiFi(); // Reconnect to WiFi if needed
  GetTimeFromGoogle(); // Get time from Google
}

void setup(){
  Serial.begin(115200);

  // Initialize alarm pin
  pinMode(ALARM_PIN, OUTPUT);
  digitalWrite(ALARM_PIN, LOW);

  // Initialize LED pin wifi connected
  pinMode(LED_WIFI, OUTPUT);
  digitalWrite(LED_WIFI, LOW);

  // Initialize status LED pin
  pinMode(STATUS_LED, OUTPUT);
  digitalWrite(STATUS_LED, LOW);
  
  pinMode(ALARM_PIN, OUTPUT);

  // Initialize RTC
  Wire.begin();
  if (!rtc.begin()){
    Serial.println("RTC FAILURE");
    BlinkError(3);
  }

  // Connect to WiFi
  ConnectToWiFi();

  // Get initial time
  GetTimeFromGoogle();
}
