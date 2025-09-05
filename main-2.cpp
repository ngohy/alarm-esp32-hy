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
#include <Max72xxPanel.h>
// #include <MD_MAX72xx.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <RTClib.h>

// #define CS_PIN 0     // Chip Select (D3)
// #define CLK_PIN 14   // Clock (D5)
// #define DIN_PIN 13   // Data In (D7)

// // for NodeMCU 1.0
// #define DIN_PIN 0  // D3
// #define CS_PIN  13  // D7
// #define CLK_PIN 12  // D6

// #define ALARM_PIN 14  // D5
// #define LED_WIFI 15  // D8

// Corrected pin definitions for NodeMCU ESP8266
#define DIN_PIN 13  // D7 (MOSI)
#define CS_PIN  15  // D8 (GPIO15 - must be pulled down with 10k resistor)
#define CLK_PIN 14  // D5 (SCK)

#define ALARM_PIN 12  // D6
#define LED_WIFI 0    // D3

const char* ssid = "HuynhHuu";
const char* password = "@0898524927";

// Weather API configuration
String weatherKey = "openweathermap API";
String weatherLang = "&lang=en";
String cityID = "1597591";

WiFiClient client;
RTC_DS3231 rtc;

// Matrix configuration (4 matrices connected)
const int pinCS = CS_PIN;  // 
const int numberOfHorizontalDisplays = 4;  // 4 matrices horizontally
const int numberOfVerticalDisplays = 1;    // 1 matrix vertically
Max72xxPanel matrix = Max72xxPanel(pinCS, numberOfHorizontalDisplays, numberOfVerticalDisplays);

// Display settings
int wait = 60;
int spacer = 4;
int width = 5 + spacer;

// Time management
int h, m, s;
String date;
String weatherString;


// Timing variables
int updCnt = 0;
int dots = 0;
long dotTime = 0;
long clkTime = 0;

void GetTimeFromGoogle() {
  WiFiClient client;
  if (!client.connect("www.google.com", 80)) {
    Serial.println("Connection to Google failed");
    return;
  }

  client.print(String("GET / HTTP/1.1\r\n") +
               String("Host: www.google.com\r\n") +
               String("Connection: close\r\n\r\n"));

  int repeatCounter = 0;
  while (!client.available() && repeatCounter < 10) {
    delay(500);
    repeatCounter++;
  }

  String line;
  while (client.connected() && client.available()) {
    line = client.readStringUntil('\n');
    line.toUpperCase();
    if (line.startsWith("DATE: ")) {
      // Example: "Date: Wed, 21 Oct 2022 07:28:00 GMT"
      String timeStr = line.substring(23, 31); // Extract "07:28:00"
      h = timeStr.substring(0, 2).toInt();
      m = timeStr.substring(3, 5).toInt();
      s = timeStr.substring(6, 8).toInt();
      
      // Adjust for timezone (UTC+7)
      h = (h + 7) % 24;
      
      // Set RTC time
      rtc.adjust(DateTime(2020, 1, 1, h, m, s)); // Date doesn't matter for our use
      Serial.println("RTC set to: " + String(h) + ":" + String(m) + ":" + String(s));
      break;
    }
  }
  client.stop();
}

void DisplayTime() {
  DateTime now = rtc.now();
  
  matrix.fillScreen(LOW);
  int y = (matrix.height() - 8) / 2;
  int xh = 2;
  int xm = 19;
  //    int xs = 28;
  
  // Blinking colon
  if (now.second() % 2) {
    matrix.drawChar(14, y, (String(":"))[0], HIGH, LOW, 1);
  }
  else {
    matrix.drawChar(14, y, (String(" "))[0], HIGH, LOW, 1);
  }

  char hour1 = char (now.hour() / 10);
  char hour2 = char (now.hour() % 10);
  char min1 = char (now.minute() / 10);
  char min2 = char (now.minute() % 10);
  Serial.println("RTC set to: " + hour1 + hour2 + ":" + min1 + min2 + ":" + String(now.second()));
  delay(1000);
  // String sec1 = String (s / 10);
  // String sec2 = String (s % 10);

  matrix.drawChar(xh, y, hour1[0], HIGH, LOW, 1);
  matrix.drawChar(xh + 6, y, hour2[0], HIGH, LOW, 1);
  matrix.drawChar(xm, y, min1[0], HIGH, LOW, 1);
  matrix.drawChar(xm + 6, y, min2[0], HIGH, LOW, 1);
  //    matrix.drawChar(xs, y, sec1[0], HIGH, LOW, 1);
  //    matrix.drawChar(xs+6, y, sec2[0], HIGH, LOW, 1);

  matrix.write();
}


void DisplayText(String text) {
  matrix.fillScreen(LOW);
  Serial.println("DisplayText: " + String(matrix.height()));
  delay(1000);
  for (size_t i = 0; i < text.length(); i++) {
    int x = (matrix.width() + 1) - ((matrix.width()) - i * (width - 1));
    int y = (matrix.height() - 8) / 2;
    matrix.drawChar(x, y, text[i], HIGH, LOW, 1);
    matrix.write();
  }
}

void ScrollText(String text) {
  Serial.println("DisplayText: " + String(matrix.height()));
  delay(1000);
  for (size_t i = 0; i < width * text.length() + matrix.width() - 1 - spacer; i++) {
    matrix.fillScreen(LOW);
    int x = (matrix.width() - 1) - i % width;
    int y = (matrix.height() - 8) / 2;
    int letter = i / width;

    while (x + width - spacer >= 0 && letter >= 0) {
      if (letter < static_cast<int>(text.length())) {
        matrix.drawChar(x, y, text[letter], HIGH, LOW, 1);
      }
      letter--;
      x -= width;
    }
    matrix.write();
    delay(wait);
  }
}

void getWeatherData() {
  if (client.connect("api.openweathermap.org", 80)) {
    client.println(String("GET /data/2.5/weather?id=") + cityID + 
                  "&units=metric&appid=" + weatherKey + weatherLang + 
                  "\r\nHost: api.openweathermap.org\r\nConnection: close\r\n\r\n");
  } else {
    Serial.println("Weather connection failed");
    return;
  }

  String line;
  while (client.connected() && client.available()) {
    line += client.readStringUntil('\n');
  }
  client.stop();

  DynamicJsonBuffer jsonBuf;
  JsonObject &root = jsonBuf.parseObject(line);
  if (root.success()) {
    weatherString = "Weather: " + root["weather"][0]["description"].as<String>();
  }
}

// Alarm configuration
int alarm1Hour = 6;     // First alarm hour (24-hour format)
int alarm1Minute = 54;  // First alarm minute
int alarm2Hour = 7;     // Second alarm hour (24-hour format)
int alarm2Minute = 00;   // Second alarm minute
bool alarmTriggered1 = false;
bool alarmTriggered2 = false;
unsigned long alarmStartTime = 0;
const int ALERT_DURATION = 35000; // 15 seconds alarm duration

void CheckAlarm() {
  DateTime now = rtc.now();
  
  // Check if alarm is currently active
  if (alarmStartTime > 0) {
    if (millis() - alarmStartTime < ALERT_DURATION) {
      if ((millis() / 500) % 2) {
        DisplayText("ALARM!");
      } else {
        matrix.fillScreen(LOW);
        matrix.write();
      }
      digitalWrite(ALARM_PIN, HIGH);
      delay(100);
      digitalWrite(ALARM_PIN, LOW);
      delay(100);
    } else {
      // Alarm duration complete
      digitalWrite(ALARM_PIN, LOW);
      alarmStartTime = 0;
      alarmTriggered1 = false;
      alarmTriggered2 = false;
    }
    return;
  }

  // Check alarm 1
  if (now.hour() == alarm1Hour && now.minute() == alarm1Minute && !alarmTriggered1) {
    alarmTriggered1 = true;
    alarmStartTime = millis();
    Serial.println("Alarm 1 Triggered!");
  }
  
  // Check alarm 2
  if (now.hour() == alarm2Hour && now.minute() == alarm2Minute && !alarmTriggered2) {
    alarmTriggered2 = true;
    alarmStartTime = millis();
    Serial.println("Alarm 2 Triggered!");
  }
}

void TestFourMatrix() {
  matrix.fillScreen(LOW);  // Clear all matrices

  // Test first matrix with "1"
  matrix.drawChar(0, 0, '1', HIGH, LOW, 1); 
  matrix.write();
  Serial.println("matrix 1!");
  delay(1000);  // Display for 500ms
  
  // Test second matrix with "2"
  matrix.fillScreen(LOW);  // Clear screen before writing to the next matrix
  matrix.drawChar(8, 0, '2', HIGH, LOW, 1);
  matrix.write();
  Serial.println("matrix 2!");
  delay(1000);  // Display for 500ms
  
  // Test third matrix with "3"
  matrix.fillScreen(LOW);  // Clear screen before writing to the next matrix
  matrix.drawChar(16, 0, '3', HIGH, LOW, 1);
  matrix.write();
  Serial.println("matrix 3!");
  delay(1000);  // Display for 500ms

  // Test fourth matrix with "4"
  matrix.fillScreen(LOW);  // Clear screen before writing to the next matrix
  matrix.drawChar(24, 0, '4', HIGH, LOW, 1);
  matrix.write();
  Serial.println("matrix 4!");
  delay(1500);  // Display for 500ms

  // Test all matrices with a simple sequence: "1234"
  matrix.fillScreen(LOW);  // Clear screen
  matrix.drawChar(0, 0, '1', HIGH, LOW, 1);
  matrix.drawChar(8, 0, '2', HIGH, LOW, 1);
  matrix.drawChar(16, 0, '3', HIGH, LOW, 1);
  matrix.drawChar(24, 0, '4', HIGH, LOW, 1);
  matrix.write();
  Serial.println("All matrices test!");
  delay(1000);  // Display the full number sequence for 1 second

  // Clear the screen
  matrix.fillScreen(LOW);
  matrix.write();
}

void loop() {
  // Update weather and time every 10 cycles
  if (updCnt <= 0) {
    updCnt = 10;
    getWeatherData();
    GetTimeFromGoogle();
    clkTime = millis();
  }

  // Scroll weather info every 5 seconds (when no alarm active)
  // if (millis() - clkTime > 5000 && alarmStartTime == 0) {
  //   ScrollText("ngo hy hello hello");
  //   updCnt--;
  //   clkTime = millis();
  // }

  // Handle alarms
  CheckAlarm();
  DisplayTime();
  

  // Blink colon every second
  if (millis() - dotTime > 500) {
    dotTime = millis();
    dots = !dots;
  }
}

void setup() {
  Serial.begin(115200);
  
  // TestFourMatrix();

  // Initialize alarm pin
  pinMode(ALARM_PIN, OUTPUT);
  digitalWrite(ALARM_PIN, LOW);

  // Initialize LED pin wifi connected
  pinMode(LED_WIFI, OUTPUT);
  digitalWrite(LED_WIFI, LOW);

  // Initialize RTC
  Wire.begin();
  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }

  // Initialize matrix display
  matrix.setIntensity(8);
  matrix.setRotation(0, 1);
  matrix.setRotation(1, 1);
  matrix.setRotation(2, 1);
  matrix.setRotation(3, 1);

  // Connect to WiFi
  Serial.begin(115200);

  // Connect to WiFi
  WiFi.begin(ssid, password);
  int retryCount = 0;
  while (WiFi.status() != WL_CONNECTED && retryCount < 10){
    digitalWrite(LED_WIFI, !digitalRead(LED_WIFI));
    delay(100);
    Serial.print(".");
    retryCount++;
  }

  digitalWrite(LED_WIFI, LOW);
  Serial.println("WiFi connected");

  // Get initial time
  GetTimeFromGoogle();
  Serial.print(".");
}
