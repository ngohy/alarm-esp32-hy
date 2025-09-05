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

// // for NodeMCU 1.0
// #define DIN_PIN 0  // D3
// #define CS_PIN  13  // D7
// #define CLK_PIN 12  // D6

// Corrected pin definitions for NodeMCU ESP8266
// #define DIN_PIN 13 // D7 (MOSI)
// #define CS_PIN 15  // D8 (GPIO15 - must be pulled down with 10k resistor)
// #define CLK_PIN 14 // D5 (SCK)

#include <ESP8266WiFi.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Max72xxPanel.h>
// #include <MD_MAX72xx.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <RTClib.h>

#define PIN_CS 0     // Chip Select (D3)
#define PIN_CLK 14   // Clock (D5)
#define PIN_DIN 13   // Data In (D7)

const char* ssid = "HuynhHuu";
const char* password = "@0898524927";

// Weather API configuration
String weatherKey = "openweathermap API";
String weatherLang = "&lang=en";
String cityID = "1597591";

WiFiClient client;
RTC_DS3231 rtc;

#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES 4

// Matrix configuration (4 matrices connected)
const int pinCS = 0;  // Use the same pin for all matrices (chip select for first matrix)
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

// Alarm configuration
int alarm1Hour = 6;     // First alarm hour (24-hour format)
int alarm1Minute = 54;  // First alarm minute
int alarm2Hour = 7;     // Second alarm hour (24-hour format)
int alarm2Minute = 0;   // Second alarm minute
int alarmPin = D4;      // Alarm output pin
bool alarmTriggered1 = false;
bool alarmTriggered2 = false;
unsigned long alarmStartTime = 0;
const int ALERT_DURATION = 15000; // 15 seconds alarm duration

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
  
  // Blinking colon
  if (now.second() % 2) {
    matrix.drawChar(14, y, ':', HIGH, LOW, 1);
  } else {
    matrix.drawChar(14, y, ' ', HIGH, LOW, 1);
  }

  matrix.drawChar(2, y, (now.hour() / 10) + '0', HIGH, LOW, 1);
  matrix.drawChar(8, y, (now.hour() % 10) + '0', HIGH, LOW, 1);
  matrix.drawChar(19, y, (now.minute() / 10) + '0', HIGH, LOW, 1);
  matrix.drawChar(25, y, (now.minute() % 10) + '0', HIGH, LOW, 1);
  
  matrix.write();
}


void DisplayText(String text) {
  matrix.fillScreen(LOW);
  for (size_t i = 0; i < text.length(); i++) {
    int x = (matrix.width() + 1) - ((matrix.width()) - i * (width - 1));
    int y = (matrix.height() - 8) / 2;
    matrix.drawChar(x, y, text[i], HIGH, LOW, 1);
    matrix.write();
  }
}

void ScrollText(String text) {
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

void CheckAlarm() {
  DateTime now = rtc.now();
  
  // Check if alarm is currently active
  if (alarmStartTime > 0) {
    if (millis() - alarmStartTime < ALERT_DURATION) {
      // Active alarm - blink display and sound buzzer
      if ((millis() / 500) % 2) {
        DisplayText("ALARM!");
      } else {
        matrix.fillScreen(LOW);
        matrix.write();
      }
      digitalWrite(alarmPin, HIGH);
    } else {
      // Alarm duration complete
      digitalWrite(alarmPin, LOW);
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
  if (millis() - clkTime > 5000 && alarmStartTime == 0) {
    ScrollText(weatherString);
    updCnt--;
    clkTime = millis();
  }

  // Handle alarms
  CheckAlarm();
  
  // Display time (unless alarm is active)
  if (alarmStartTime == 0) {
    DisplayTime();
  }

  // Blink colon every second
  if (millis() - dotTime > 500) {
    dotTime = millis();
    dots = !dots;
  }
}

void setup() {
  Serial.begin(115200);
  
  TestFourMatrix();

  // Initialize alarm pin
  pinMode(alarmPin, OUTPUT);
  digitalWrite(alarmPin, LOW);

  // Initialize RTC
  Wire.begin();
  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }

  // Initialize matrix display
  matrix.setIntensity(13);
  matrix.setRotation(0, 1);
  matrix.setRotation(1, 1);
  matrix.setRotation(2, 1);
  matrix.setRotation(3, 1);

  // Connect to WiFi
  Serial.begin(115200);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  delay(1000);  // Add a delay to stabilize Serial
  Serial.println("WiFi connected");

  // Get initial time from Google and set RTC
  GetTimeFromGoogle();
  Serial.print(".");
}
