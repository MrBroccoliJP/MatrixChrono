/*
 * MatrixChrono - IoT Smart Home Clock System
 * Copyright (c) 2025 João Fernandes
 * 
 * This work is licensed under the Creative Commons Attribution-NonCommercial 
 * 4.0 International License. To view a copy of this license, visit:
 * http://creativecommons.org/licenses/by-nc/4.0/
 */

#include <WiFiManager.h>  // https://github.com/tzapu/WiFiManager
#include <WebServer.h>
#include <WiFiClient.h>
WiFiClient wifiClient;
#include <CircularBuffer.hpp>
#include <ArduinoOTA.h>
#include <Adafruit_GFX.h>
#include <Adafruit_NeoMatrix.h>
#include <Adafruit_NeoPixel.h>
#include <math.h>
#include "MiniFont.h"
#include "Normal.h"
#include "Square.h"
#include "html.h"
#include "secrets.h"
#include <ArduinoJson.h>
#include <Fonts/TomThumb.h>
#define PIN 33  //more sable than pin 25, pin 25 showed some noise that was interpreted as data in the matrix.
#include <RTClib.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <StreamUtils.h>
#include "time.h"
#include <OneWire.h>
#include <ezTime.h>
#include <DallasTemperature.h>
#include <Preferences.h>
Preferences preferences;
RTC_DS3231 rtc;
Timezone country;

#define PT_POSIX "WET0WEST,M3.5.0/1,M10.5.0/2"

char* C_POSIX = PT_POSIX;

const char* firmwareVersion = "5.53";


Adafruit_NeoMatrix matrix = Adafruit_NeoMatrix(8, 8, 4, 1, PIN,
                                               NEO_TILE_TOP + NEO_TILE_LEFT + NEO_TILE_ROWS + NEO_TILE_PROGRESSIVE + NEO_MATRIX_TOP + NEO_MATRIX_LEFT + NEO_MATRIX_ROWS + NEO_TILE_PROGRESSIVE,
                                               NEO_GRB + NEO_KHZ800);
#include "icons.h"  //this needs to be after the matrix declaration since it uses matrix stuff


const int oneWireBus = 32;
OneWire oneWire(oneWireBus);
DallasTemperature sensors(&oneWire);
WebServer server(80);

// --------------------------------------------------
// Persistent settings
// --------------------------------------------------
struct AppSettings {
  uint8_t maxbrightness;

  uint16_t mainColor;   // RGB565
  uint16_t nightColor;  // RGB565

  bool randomColor;
  bool weather;

  char city[32];
  char language[16];

  uint8_t nightModeStartHour;
  uint8_t nightModeStartMin;
  uint8_t nightModeEndHour;
  uint8_t nightModeEndMin;
};

AppSettings settings;

uint8_t red, green, blue;  //USED FOR THE RGB565TORGB FUNCTION

uint8_t r = 0;             //main red color
uint8_t g = 0;             //main green color
uint8_t b = 0;             //main blue color

uint8_t maxChannelValue;
uint8_t targetR, targetG, targetB;                           //used to store the max value of r,g,b in order to calculate a random color with the same "brightness"
float targetStepR, targetStepG, targetStepB;                 //these values are used to calculate the random color change
float interpolatedR, interpolatedG, interpolatedB;  //float values to temporarily calculate the values of r,g,b if random color
float fadeCurrentR = 0;                                          //used for calculation for red in tick effect
float fadeCurrentG = 0;                                          //used for calculation for green in tick effect
float fadeCurrentB = 0;                                          //used for calculation for blue in tick effect
float fadeStepR= 0;                                          //used for calculation of the increments for red
float fadeStepG = 0;                                          //used for calculation of the increments for green
float fadeStepB = 0;                                          //used for calculation of the increments for blue


enum class ClockMode { NORMAL,
                       NIGHT };
ClockMode currentMode;
bool modeInitialized = false;

const uint8_t buttonPin = 26;

uint16_t currentColor = 0;

unsigned long tmp_millis = millis();  //used to store millis information
unsigned long wifi_millis = 0;
unsigned long int showTempMillis;
uint8_t lastTempReading ;

//wEATHER VARIABLES
uint8_t last_weather_update_hour = 25;

String weather_str;
short int sunrise_h = 8;
short int sunrise_m = 00;  //set to 08:00 in case it fails to get weather and does not update it.


//BUTTON DEBOUNCE
bool buttonState = HIGH;             // Current state of the button
bool lastButtonState = HIGH;         // Previous state of the button
unsigned long lastDebounceTime = 0;  // Last time the button state changed
bool wifiConnected = false;



uint8_t clockHour;
uint8_t clockMin;
int8_t screenClockHour = -1;
int8_t screenClockMin = -1;
uint8_t clockSec;  //only used for certain functions.
short int LastRTCSyncTime = -1;


bool sunrise_loop = 0;

CircularBuffer<uint8_t, 100> tempDataHour;
CircularBuffer<uint8_t, 100> tempDataMin;
CircularBuffer<float, 100> tempDataSensor;  //Actual sensor
CircularBuffer<String, 20> terminalBuffer;

WiFiManager wm;

void setup() {
  String outText;
  sensors.begin();

  pinMode(buttonPin, INPUT_PULLUP);
  Serial.begin(115200);
  loadSettings(settings);
  terminalOutput("Settings loaded from nvs");
  //initialization
  matrix.begin();
  matrix.setTextWrap(false);
  matrix.setBrightness(settings.maxbrightness);
  matrix.fillScreen(0);
  matrix.setFont(&Normal);
  matrix.setTextSize(1);
  outText = "V" + String(firmwareVersion);
  matrix.setCursor(0, 7);
  matrix.print(outText);
  matrix.show();
  delay(1000);
  fadeOut(500);  //fadeout in milliseconds
  matrix.fillScreen(0);

  WiFi.mode(WIFI_STA);  // explicitly set mode, esp defaults to STA+AP
  WiFi.setSleep(WIFI_PS_NONE);
  WiFi.setAutoReconnect(true);
  WiFi.persistent(true);              // keep WiFi config in NVS
  wm.setConfigPortalBlocking(false);  // Set configuration portal to non-blocking mode
  wm.setConfigPortalTimeout(120);     // Set configuration portal timeout to 60 seconds

  // Attempt to connect using saved credentials
  //wifiportal = wm.autoConnect("NEOCLOCK_AP");
  wm.autoConnect("NEOCLOCK_AP");
  //delay for wifi connection?
  matrix.setBrightness(settings.maxbrightness);
  // outText = "WiFi Connecting";
  // matrix.setCursor(0, 8);
  // matrix.print(outText);
  // matrix.show();
  for (int i = 0; i < 10; i++) {

    displayIcon(wifi_frame1, 11, 1);
    delay(100);
    displayIcon(wifi_frame2, 11, 1);
    delay(100);
    displayIcon(wifi_frame3, 11, 1);
    delay(100);
    displayIcon(wifi_frame4, 11, 1);
    delay(100);
  }


  if (WiFi.status() == WL_CONNECTED) {
    terminalOutput("Connected to WiFi using saved credentials");
    //Serial.println("Connected to WiFi using saved credentials");

    wifiConnected = 1;
  } else {
    wifiConnected = 0;

    //Serial.println("Failed to connect to WiFi using saved credentials");
    terminalOutput("Failed to connect to WiFi using saved credentials");
    terminalOutput("Configuration portal running");
    //Serial.println("Configuration portal running");
    displayIcon(no_wifi, 11, 1);
    delay(2000);
  }
  fadeOut(500);

  ArduinoOTA.setPassword("admin");

  ArduinoOTA
    .onStart([]() {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH)
        type = "sketch";
      else  // U_SPIFFS
        type = "filesystem";

      // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
      Serial.println("Start updating " + type);
    })
    .onEnd([]() {
      Serial.println("\nEnd");
    })
    .onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    })
    .onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });

  ArduinoOTA.begin();


  //PANIC THIS PUTS THE DEVICE INTO A LOOP SO THAT A CODE CAN BE UPLOADED
  int reading = digitalRead(buttonPin);
  if (reading == LOW) {
    matrix.setBrightness(settings.maxbrightness);
    ArduinoOTA.handle();
    matrix.fillScreen(0);
    matrix.setCursor(0, 7);
    matrix.print("update");
    matrix.show();
    delay(1000);
    while (1) {
      int rt = 0;
      int gt = 0;
      int bt = 0;
      int led_n = 0;
      while (led_n < 256) {
        ArduinoOTA.handle();
        r = random(0, 255);
        g = random(0, 255);
        b = random(0, 255);
        while (rt < r || gt < g || bt < b) {
          if (rt < r) {
            rt = rt + 5;
          }
          if (gt < g) {
            gt = gt + 5;
          }
          if (bt < b) {
            bt = bt + 5;
          }
          matrix.setPixelColor(led_n, rt, gt, bt);
          matrix.show();
        }

        rt = 0;
        gt = 0;
        bt = 0;

        led_n++;
      }
      matrix.fillScreen(0);
      led_n = 0;
    }
  }

  tmp_millis = millis();

  //display IP
  matrix.setBrightness(settings.maxbrightness);
  scrollText(WiFi.localIP().toString(), 50);

  //Serial.println("Setup1" + wifiConnected);
  terminalOutput("Setup Flag1 - wifiConnected:" + String(wifiConnected));


  rtc.begin();
  country.setPosix(C_POSIX);


  if (wifiConnected) {
    //if the wifi is connected, set time from WEB
    waitForSync();
    setInterval(0);  //disable the sync timer from eztime~THIS NEEDS TO BE AFTER THE Sync for some reason

    terminalOutput("time updated: " + String(lastNtpUpdateTime()));
    clockSec = country.second();
    while (clockSec == country.second()) {  //THIS IS USED TO SYNC THE SECONDS OF THE RTC WITH THE SECONDS FROM EZTIME
      yield();
    }
    rtc.adjust(DateTime(country.year(), country.month(), country.day(), country.hour(), country.minute(), country.second()));
    LastRTCSyncTime = country.hour() * 1000 + country.minute();

  } else {
    DateTime now = rtc.now();
    //if the wifi is not connected set time from rtc
    country.setTime(now.hour(), now.minute(), now.second(), now.day(), now.month(), now.year());
    terminalOutput("time set from rtc clock since wifi not connected");
  }


  startWebServers();
  sensors.requestTemperatures();
  DateTime now = rtc.now();

  terminalOutput("Setup end");
}



void loop() {
  wm.process();
  handleButton();
  ArduinoOTA.handle();

  handleWifi();

  // Get the current time
  int currentTime = country.hour() * 100 + country.minute();
  int nightEndTime = settings.nightModeEndHour * 100 + settings.nightModeEndMin;
  int nightStartTime = settings.nightModeStartHour * 100 + settings.nightModeStartMin;

  bool isNight;
  if (nightStartTime == nightEndTime) {
    isNight = false;
  } else if (nightStartTime > nightEndTime) {  // crosses midnight
    isNight = (currentTime >= nightStartTime) || (currentTime < nightEndTime);
  } else {
    isNight = (currentTime >= nightStartTime) && (currentTime < nightEndTime);
  }

  ClockMode newMode = isNight ? ClockMode::NIGHT : ClockMode::NORMAL;

  if (newMode != currentMode || !modeInitialized) {
    currentMode = newMode;
    modeInitialized = true;
    matrix.fillScreen(0);

    if (currentMode == ClockMode::NIGHT) {
      currentColor = settings.nightColor;
      matrix.setFont(&Mini_Font);
      matrix.setTextColor(currentColor);
      RGB565TORGB(settings.nightColor, r, g, b);
      fadeStepR= double(r) / 25;
      fadeStepG = double(g) / 23;
      fadeStepB = double(b) / 24;
      getWeather();

    } else {
      sunrise_loop = 0;
      currentColor = settings.mainColor;
      matrix.setFont(&square_big_font);
      matrix.setTextColor(currentColor);
      RGB565TORGB(settings.mainColor, r, g, b);
      fadeStepR= double(r) / 25;
      fadeStepG = double(g) / 23;
      fadeStepB = double(b) / 24;
      maxChannelValue = max(max(r, g), b);
      getWeather();
    }
  }

  if (currentMode == ClockMode::NIGHT) {
    minimalistClock();
  } else {
    normalClock();
  }

  yield();
}

void handleWifi() {
  // Not connected — attempt reconnect every 2 minutes
  if (WiFi.status() != WL_CONNECTED && (wifi_millis == 0 || millis() - wifi_millis > 120000)) {
    wifiConnected = false;
    server.close();
    matrix.drawPixel(31, 0, 0xf800);  // red pixel = no wifi
    terminalOutput("WiFi not connected. Attempting to reconnect...");
    wm.setConfigPortalTimeout(120);
    wm.autoConnect("NEOCLOCK_AP");
    wifi_millis = millis();
    return;
  }

  // Just (re)connected — verify web server is reachable, restart if not
  if (WiFi.status() == WL_CONNECTED && !wifiConnected) {
    matrix.drawPixel(31, 0, 0x0000);  // clear red pixel
    if (!wifiClient.connect(WiFi.localIP(), 80) && millis() - wifi_millis > 60000) {
      server.close();
      startWebServers();
      terminalOutput("Web server restarted after reconnection");
      wifi_millis = millis();
    } else {
      wifiConnected = true;
    }
    return;
  }

  // Connected and running — handle clients, watchdog check server health
  if (WiFi.status() == WL_CONNECTED && wifiConnected) {
    server.handleClient();
    if (!wifiClient.connect(WiFi.localIP(), 80) && millis() - wifi_millis > 60000) {
      wifiConnected = false;
      server.close();
      startWebServers();
      terminalOutput("Web server restarted - was unresponsive");
    }
  }
}

void normalClock() {
  short int currentBrightness = matrix.getBrightness();
  unsigned long int MillisKeep;
  while (currentBrightness != settings.maxbrightness) {
    if (currentBrightness < settings.maxbrightness) {
      for (int i = currentBrightness; i < settings.maxbrightness + 1;) {
        matrix.setBrightness(i);
        refreshNormalClock();
        fadeTick(16, 2, 6);
        currentBrightness = matrix.getBrightness();
        //Serial.println("++Brightness level: " + String(i));
        i++;
        delay(5);
      }
    } else if (currentBrightness > settings.maxbrightness) {
      for (int i = currentBrightness; i > settings.maxbrightness - 1;) {
        matrix.setBrightness(i);
        refreshNormalClock();
        fadeTick(16, 2, 6);
        currentBrightness = matrix.getBrightness();
        //Serial.println("--Brightness level: " + String(i));
        i--;
        delay(5);
      }
    }
  }

  if (screenClockHour == country.hour() && screenClockMin == country.minute()) {
    handleButton();
    ArduinoOTA.handle();
    server.handleClient();
    fadeTick(16, 2, 6);
    //matrix.show();
  } else {  //CLOCK CHANGED
    fadeTick(16, 2, 6);
    refreshNormalClockSlideEffect();
    //temperature display every 15min starting from 5 -> 20 -> 35 -> 50
    if (clockMin % 15 == 5) {

      MillisKeep = millis();
      while (millis() - MillisKeep < 5000) {
        fadeTick(16, 2, 6);
        //matrix.show();
      }  //delay 5 seconds

      getWeather();
      fadeOut(100);
      matrix.fillScreen(0);
      matrix.setBrightness(settings.maxbrightness);
      matrix.setFont(&Normal);
      scrollText(weather_str, 50);
      matrix.setFont(&square_big_font);

      //read sensor data
      sensors.requestTemperatures();
      tempDataHour.push(country.hour());
      tempDataMin.push(country.minute());
      tempDataSensor.push(sensors.getTempCByIndex(0));

      //delay(5000);  //delay 5 seconds
      fadeOut(100);
      matrix.fillScreen(0);
      matrix.setBrightness(settings.maxbrightness);
      matrix.setCursor(0, 7);
      matrix.print(String(int(tempDataSensor.last())) + "$C");  //$ = º --> used $ to save space on the font
      matrix.show();
      screenClockMin = -1;
      screenClockHour = -1;
      syncSecondsWithRTC();
      delay(5000);   //display temperature for 5 seconds
      fadeOut(100);  //fadeout temperature
      matrix.fillScreen(0);
      matrix.setBrightness(settings.maxbrightness);
      refreshNormalClockSlideEffect();
    }
  }
}

void refreshNormalClock() {  //without any effect
  // Serial.println("refreshNormalClock");
  short int start_cursor = 4;
  short int font_width = 5;
  font_width++;  //to account for the space between characters
  short int cursor = start_cursor;

  clockHour = country.hour();
  clockMin = country.minute();
  screenClockHour = clockHour;
  screenClockMin = clockMin;
  matrix.fillScreen(0);
  matrix.setCursor(cursor, 7);
  matrix.print(String(screenClockHour / 10));
  cursor = cursor + font_width;  // Move to the next digit position
  matrix.setCursor(cursor, 7);
  matrix.print(String(screenClockHour % 10));
  cursor = cursor + font_width;
  cursor = cursor + 2;  // Account for spacing between hour and minute digits
  matrix.setCursor(cursor, 7);
  matrix.print(String(screenClockMin / 10));
  cursor = cursor + font_width;  // Move to the next digit position
  matrix.setCursor(cursor, 7);
  matrix.print(String(screenClockMin % 10));
  matrix.show();
}


void refreshNormalClockSlideEffect() {
  // Serial.println("refreshNormalClockSlideEffect");
  // Initialize variables
  short int start_cursor = 4;
  short int font_width = 5;
  font_width++;  //to account for the space between characters
  short int cursor = start_cursor;
  short int slide_timing = 5;

  // Get current time
  clockHour = country.hour();
  clockMin = country.minute();

  if (settings.randomColor) {  //THIS PART IS ONLY RUN IF RANDOM COLOR IS ENABLED
    if ((screenClockHour != clockHour && screenClockHour > 0) || (targetR + targetG + targetB) <= 10) {
      if ((interpolatedR + interpolatedG + interpolatedB) == 0) {

        interpolatedR = r;
        interpolatedG = g;
        interpolatedB = b;
      }
      randomNextColor(maxChannelValue, 60 - clockMin);  //to run every hour
      terminalOutput("targetR:" + String(targetR) + " targetG:" + String(targetG) + " targetB:" + String(targetB) + " max rgb value:" + String(maxChannelValue));
    }
    if (screenClockMin >= 0 && screenClockMin != clockMin) {  //to run every minute
      // Update red component
      interpolatedR = (interpolatedR < targetR) ? min(interpolatedR + targetStepR, float(targetR)) : max(interpolatedR - targetStepR, float(targetR));
      // Update green component
      interpolatedG = (interpolatedG < targetG) ? min(interpolatedG + targetStepG, float(targetG)) : max(interpolatedG - targetStepG, float(targetG));
      // Update blue component
      interpolatedB = (interpolatedB < targetB) ? min(interpolatedB + targetStepB, float(targetB)) : max(interpolatedB - targetStepB, float(targetB));

      fadeStepR= double(r) / 25;
      fadeStepG = double(g) / 25;
      fadeStepB = double(b) / 25;
      r = int(round(interpolatedR));
      g = int(round(interpolatedG));
      b = int(round(interpolatedB));
      currentColor = matrix.Color(r, g, b);
      matrix.setTextColor(currentColor);
      terminalOutput("r:" + String(r) + "g:" + String(g) + "b:" + String(b));
    }
  }


  // Check if the screen clock hour and minute are negative, this means the clock is not being shown on the screen so it needs a complete refresh
  if (screenClockHour < 0 || screenClockMin < 0) {
    // Serial.println("refreshNormalClockSlideEffect | screenClockHour < 0 || screenClockMin < 0");
    // Loop for slide effect animation
    for (int i = 0; i < 9; i++) {
      // Print blank character
      cursor = start_cursor;
      matrix.fillScreen(0);
      matrix.setTextColor(0);
      matrix.setCursor(cursor, i + 7);
      matrix.print("#");  //blank character
      matrix.setTextColor(currentColor);

      // Print tens digit of hour
      if (clockHour < 10) {
        matrix.setCursor(cursor, i - 1);
        matrix.setTextColor(0);
        matrix.print("#");  //blank character
        matrix.setTextColor(currentColor);
      } else {
        matrix.setCursor(cursor, i - 1);
        matrix.print(String(clockHour / 10));
      }
      cursor = cursor + font_width;  // Move to the next digit position

      // Print ones digit of hour
      matrix.setCursor(cursor, i + 7);
      matrix.setTextColor(0);
      matrix.print("#");
      matrix.setTextColor(currentColor);
      matrix.setCursor(cursor, i - 1);
      matrix.print(String(clockHour % 10));
      cursor = cursor + font_width;

      // Print space between hour and minute
      cursor = cursor + 2;

      // Print tens digit of minute
      matrix.setCursor(cursor, i + 7);
      matrix.setTextColor(0);
      matrix.print("#");
      matrix.setTextColor(currentColor);
      matrix.setCursor(cursor, i - 1);
      matrix.print(String(clockMin / 10));
      cursor = cursor + font_width;

      // Print ones digit of minute
      matrix.setCursor(cursor, i + 7);
      matrix.setTextColor(0);
      matrix.print("#");
      matrix.setTextColor(currentColor);
      matrix.setCursor(cursor, i - 1);
      matrix.print(String(clockMin % 10));
      cursor = cursor + font_width;

      // Perform fade effect and update display
      // fadeTick(16, 2, 6);
      // matrix.show();
      for (int d = 0; d < slide_timing; d++) {
        fadeTick(16, i - 2, i - 6);  //to account for the slide?
        //matrix.show();
      }
    }
  } else if (clockHour == screenClockHour && clockMin == screenClockMin) {
    for (int i = 0; i < 9; i++) {
      cursor = start_cursor;
      matrix.fillScreen(0);

      // Print tens digit of hour
      if (clockHour < 10) {
        matrix.setCursor(cursor, 7);
        matrix.setTextColor(0);
        matrix.print("#");  //blank character
        matrix.setTextColor(currentColor);
      } else {
        matrix.setCursor(cursor, 7);
        matrix.print(String(clockHour / 10));
      }

      cursor = cursor + font_width;  // Move to the next digit position

      // Print ones digit of hour
      matrix.setCursor(cursor, 7);
      matrix.print(String(clockHour % 10));
      cursor = cursor + font_width;

      // Print space between hour and minute
      cursor = cursor + 2;

      // Print tens digit of minute
      matrix.setCursor(cursor, 7);
      matrix.print(String(clockMin / 10));
      cursor = cursor + font_width;

      // Print ones digit of minute
      matrix.setCursor(cursor, 7);
      matrix.print(String(clockMin % 10));
      cursor = cursor + font_width;

      for (int d = 0; d < slide_timing; d++) {
        fadeTick(16, 2, 6);
        //matrix.show();
      }
    }
  } else if ((clockHour / 10) != screenClockHour / 10) {
    // Serial.println("refreshNormalClockSlideEffect | (clockHour / 10) != screenClockHour / 10");
    // Loop for slide effect animation if tens digit of hour changes
    for (int i = 0; i < 9; i++) {
      cursor = start_cursor;
      matrix.fillScreen(0);

      // Print tens digit of hour
      matrix.setCursor(cursor, i + 7);
      matrix.print(String(screenClockHour / 10));

      if (clockHour < 10) {
        matrix.setCursor(cursor, i - 1);
        matrix.setTextColor(0);
        matrix.print("#");  //blank character
        matrix.setTextColor(currentColor);
      } else {
        matrix.setCursor(cursor, i - 1);
        matrix.print(String(clockHour / 10));
      }

      cursor = cursor + font_width;  // Move to the next digit position

      // Print ones digit of hour
      matrix.setCursor(cursor, i + 7);
      matrix.print(String(screenClockHour % 10));
      matrix.setCursor(cursor, i - 1);
      matrix.print(String(clockHour % 10));
      cursor = cursor + font_width;

      // Print space between hour and minute
      cursor = cursor + 2;

      // Print tens digit of minute
      matrix.setCursor(cursor, i + 7);
      matrix.print(String(screenClockMin / 10));
      matrix.setCursor(cursor, i - 1);
      matrix.print(String(clockMin / 10));
      cursor = cursor + font_width;

      // Print ones digit of minute
      matrix.setCursor(cursor, i + 7);
      matrix.print(String(screenClockMin % 10));
      matrix.setCursor(cursor, i - 1);
      matrix.print(String(clockMin % 10));
      cursor = cursor + font_width;

      for (int d = 0; d < slide_timing; d++) {
        fadeTick(16, 2, 6);
        //matrix.show();
      }
    }
  } else if (clockHour % 10 != screenClockHour % 10) {
    // Serial.println("refreshNormalClockSlideEffect | clockHour % 10 != screenClockHour % 10) ");
    // Loop for slide effect animation if ones digit of hour changes
    for (int i = 0; i < 9; i++) {
      cursor = start_cursor;
      matrix.fillScreen(0);

      // Print tens digit of hour
      if (clockHour < 10) {
        matrix.setCursor(cursor, 7);
        matrix.setTextColor(0);
        matrix.print("#");  //blank character
        matrix.setTextColor(currentColor);
      } else {
        matrix.setCursor(cursor, 7);
        matrix.print(String(clockHour / 10));
      }

      cursor = cursor + font_width;  // Move to the next digit position

      // Print ones digit of hour
      matrix.setCursor(cursor, i + 7);
      matrix.print(String(screenClockHour % 10));
      matrix.setCursor(cursor, i - 1);
      matrix.print(String(clockHour % 10));
      cursor = cursor + font_width;

      // Print space between hour and minute
      cursor = cursor + 2;

      // Print tens digit of minute
      matrix.setCursor(cursor, i + 7);
      matrix.print(String(screenClockMin / 10));
      matrix.setCursor(cursor, i - 1);
      matrix.print(String(clockMin / 10));
      cursor = cursor + font_width;

      // Print ones digit of minute
      matrix.setCursor(cursor, i + 7);
      matrix.print(String(screenClockMin % 10));
      matrix.setCursor(cursor, i - 1);
      matrix.print(String(clockMin % 10));
      cursor = cursor + font_width;

      for (int d = 0; d < slide_timing; d++) {
        fadeTick(16, 2, 6);
        //matrix.show();
      }
    }
  } else if (clockMin / 10 != screenClockMin / 10) {
    // Serial.println("refreshNormalClockSlideEffect |(clockMin / 10 != screenClockMin / 10)");
    // Loop for slide effect animation if tens digit of minute changes
    for (int i = 0; i < 9; i++) {
      cursor = start_cursor;
      matrix.fillScreen(0);

      // Print tens digit of hour
      if (clockHour < 10) {
        matrix.setCursor(cursor, 7);
        matrix.setTextColor(0);
        matrix.print("#");  //blank character
        matrix.setTextColor(currentColor);
      } else {
        matrix.setCursor(cursor, 7);
        matrix.print(String(clockHour / 10));
      }

      cursor = cursor + font_width;  // Move to the next digit position

      // Print ones digit of hour
      matrix.setCursor(cursor, 7);
      matrix.print(String(clockHour % 10));
      cursor = cursor + font_width;

      // Print space between hour and minute
      cursor = cursor + 2;

      // Print tens digit of minute
      matrix.setCursor(cursor, i + 7);
      matrix.print(String(screenClockMin / 10));
      matrix.setCursor(cursor, i - 1);
      matrix.print(String(clockMin / 10));
      cursor = cursor + font_width;

      // Print ones digit of minute
      matrix.setCursor(cursor, i + 7);
      matrix.print(String(screenClockMin % 10));
      matrix.setCursor(cursor, i - 1);
      matrix.print(String(clockMin % 10));
      cursor = cursor + font_width;

      // Perform fade effect and update display
      // fadeTick(16, 2, 6);
      // matrix.show();
      for (int d = 0; d < slide_timing; d++) {
        fadeTick(16, 2, 6);
        //matrix.show();
      }
    }
  } else if (clockMin % 10 != screenClockMin % 10) {
    // Loop for slide effect animation if ones digit of minute changes
    for (int i = 0; i < 9; i++) {
      cursor = start_cursor;
      matrix.fillScreen(0);

      // Print tens digit of hour
      if (clockHour < 10) {
        matrix.setCursor(cursor, 7);
        matrix.setTextColor(0);
        matrix.print("#");  //blank character
        matrix.setTextColor(currentColor);
      } else {
        matrix.setCursor(cursor, 7);
        matrix.print(String(clockHour / 10));
      }

      cursor = cursor + font_width;  // Move to the next digit position

      // Print ones digit of hour
      matrix.setCursor(cursor, 7);
      matrix.print(String(clockHour % 10));
      cursor = cursor + font_width;

      // Print space between hour and minute
      cursor = cursor + 2;

      // Print tens digit of minute
      matrix.setCursor(cursor, 7);
      matrix.print(String(clockMin / 10));
      cursor = cursor + font_width;

      // Print ones digit of minute
      matrix.setCursor(cursor, i + 7);
      matrix.print(String(screenClockMin % 10));
      matrix.setCursor(cursor, i - 1);
      matrix.print(String(clockMin % 10));
      cursor = cursor + font_width;

      // Perform fade effect and update display
      // fadeTick(16, 2, 6);
      // matrix.show();
      for (int d = 0; d < slide_timing; d++) {
        fadeTick(16, 2, 6);
        // matrix.show();
      }
    }
  }

  // Update screen clock hour and minute
  screenClockHour = clockHour;
  screenClockMin = clockMin;
}

void tick(short int x_loc, short int y_loc_led1, short int y_loc_led2) {
  short int correction;
  if ((millis() - tmp_millis) > 500 && (millis() - tmp_millis) <= 1000) {
    fadeCurrentR = r;
    fadeCurrentG = g;
    fadeCurrentB = b;
  } else if ((millis() - tmp_millis) < 500 && (millis() - tmp_millis) <= 1000) {
    fadeCurrentR = 0;
    fadeCurrentG = 0;
    fadeCurrentB = 0;

  } else if (((millis() - tmp_millis) > 1000) && ((millis() - tmp_millis) < 1999)) {
    correction = 1000 - (millis() - tmp_millis);
    tmp_millis = millis() - correction;
  } else if ((millis() - tmp_millis) >= 2000) {
    tmp_millis = millis() + country.ms();  //correct it with ms from eztime
                                           // tmp_millis = millis();
  }

  uint16_t color = matrix.Color(int(fadeCurrentR), int(fadeCurrentG), int(fadeCurrentB));
  matrix.drawPixel(x_loc, y_loc_led1, color);
  matrix.drawPixel(x_loc, y_loc_led2, color);
  matrix.show();
}

void fadeTick(short int x_loc, short int y_loc_led1, short int y_loc_led2) {

  short int correction;

  if ((millis() - tmp_millis) > 500 && (millis() - tmp_millis) <= 1000) {
    if (fadeCurrentR != r) {
      fadeStepR= double(r) / 23;
      fadeCurrentR = (fadeCurrentR + fadeStepR<= r) ? (fadeCurrentR + fadeStepR) : r;
    }
    if (fadeCurrentG != g) {
      fadeStepG = double(g) / 25;
      fadeCurrentG = (fadeCurrentG + fadeStepG <= g) ? (fadeCurrentG + fadeStepG) : g;
    }
    if (fadeCurrentB != b) {
      fadeStepB = double(b) / 24;
      fadeCurrentB = (fadeCurrentB + fadeStepB <= b) ? (fadeCurrentB + fadeStepB) : b;
    }
    //fade in tick until 500miliseconds (1/2 second)
  } else if ((millis() - tmp_millis) < 500 && (millis() - tmp_millis) <= 1000) {
    if (fadeCurrentR != 0) {
      fadeStepR= double(r) / 25;
      fadeCurrentR = (fadeCurrentR - fadeStepR>= 0) ? (fadeCurrentR - fadeStepR) : 0;
    }
    if (fadeCurrentG != 0) {
      fadeStepG = double(g) / 23;
      fadeCurrentG = (fadeCurrentG - fadeStepG >= 0) ? (fadeCurrentG - fadeStepG) : 0;
    }
    if (fadeCurrentB != 0) {
      fadeStepB = double(b) / 24;
      fadeCurrentB = (fadeCurrentB - fadeStepB >= 0) ? (fadeCurrentB - fadeStepB) : 0;
    }
    //fade out tick until 1000miliseconds (full second)
  } else if (((millis() - tmp_millis) > 1000) && ((millis() - tmp_millis) < 1999)) {
    //in this case tmp_millis might be misaligned with the seconds, so this tries to correct it.
    correction = 1000 - (millis() - tmp_millis);
    tmp_millis = millis() - correction;
  } else if ((millis() - tmp_millis) >= 2000) {
    tmp_millis = millis() + country.ms();  //correct it with ms from eztime
  }

  uint16_t color = matrix.Color(int(fadeCurrentR), int(fadeCurrentG), int(fadeCurrentB));
  matrix.drawPixel(x_loc, y_loc_led1, color);
  matrix.drawPixel(x_loc, y_loc_led2, color);
  matrix.show();
}

void randomNextColor(uint8_t maxChannelValue, int minutes_to_target) {
  // Generate a random component value equal to maxChannelValue
  uint8_t randomComponentIndex = random(3);
  targetR = (randomComponentIndex == 0) ? maxChannelValue : random(maxChannelValue);
  targetG = (randomComponentIndex == 1) ? maxChannelValue : random(maxChannelValue);
  targetB = (randomComponentIndex == 2) ? maxChannelValue : random(maxChannelValue);

  targetStepR = (float)abs(r - targetR) / minutes_to_target;
  targetStepG = (float)abs(g - targetG) / minutes_to_target;
  targetStepB = (float)abs(b - targetB) / minutes_to_target;
}

void minimalistClock() {

  matrix.setBrightness(settings.maxbrightness);
  //matrix.setTextColor(matrix.Color(r, g, b));
  //bool temperatureDisplayed = false
  short int tickXLoc;
  short int cursor = 0;

  if (country.hour() < 10) {
    tickXLoc = 4;
  } else {
    tickXLoc = 8;
  }

  if (clockHour == country.hour() && clockMin == country.minute()) {  //clockHour and clockMin didnt change
    minimalisticTick(tickXLoc, 4, 6);

    if (clockMin % 15 == 5) {  //every 15 minutes
      if (clockHour < 10) {    //cursor changes depending on the digits of the clock since before 10h it is offset by 4
        cursor = 17;           //the c gets cut off but at least it fits
      } else {
        cursor = 20;
      }
      if (lastTempReading  != clockMin) {
        // this part is only supposed to run once every 15 minutes
        sensors.requestTemperatures();  //request temperatures twice because the sensor seems to retain info from the last reading in the first request
        sensors.requestTemperatures();
        tempDataHour.push(country.hour());                //push hour to an array that stores the 100 time and temperatures
        tempDataMin.push(country.minute());               //push minutes
        tempDataSensor.push(sensors.getTempCByIndex(0));  //push temperature

        lastTempReading  = clockMin;                                     //update the last reading time
        matrix.setCursor(cursor, 4);                                     //set the cursor for the matrix print.
        matrix.print(String(int(round(tempDataSensor.last()))) + "ºC");  //print on the screen the temperature
        showTempMillis = millis();                                             //the start time of the temperature display
        if (showTempMillis == 0) {
          showTempMillis = 1;  //in the unlikely event (1milisecond is not gonna matter)
        }
      }
      if (millis() - showTempMillis > 5000 && showTempMillis != 0) {  //after 5 seconds it should clear the matrix temperature data.
        matrix.setCursor(cursor, 4);
        matrix.setTextColor(0);
        matrix.print("####");
        matrix.setTextColor(currentColor);
        //Serial.println("CLEARING SCREEN");
        showTempMillis = 0;
      }
    }

  } else {  //clock changed

    if (clockMin % 35 == 0) {
      syncSecondsWithRTC();  //sync seconds every hour at the 35 min
    }

    clockHour = country.hour();
    clockMin = country.minute();
    int nowMinutes = clockHour * 60 + clockMin;
    int sunriseMinutes = (sunrise_h * 60 + sunrise_m) - 15;  //we want the loop to start 15 min before the sunrise hour
    int sunriseEndMinutes = sunriseMinutes + 60;
    //if the time is between the sunset window.
    if (nowMinutes >= sunriseMinutes && nowMinutes < sunriseEndMinutes) {

      if (sunrise_loop == 0) {  //first run
        // Convert colors
        RGB565TORGB(settings.mainColor, targetR, targetG, targetB);
        RGB565TORGB(settings.nightColor, r, g, b);

        //calculate minutes to reach target
        int dec_target_time = sunriseEndMinutes - nowMinutes;
        if (dec_target_time < 1) dec_target_time = 1;
        // Calculate step sizes for each color component
        targetStepR = (float)abs(r - targetR) / dec_target_time;
        targetStepG = (float)abs(g - targetG) / dec_target_time;
        targetStepB = (float)abs(b - targetB) / dec_target_time;

        // Initialize calculation variables
        interpolatedR = r;
        interpolatedG = g;
        interpolatedB = b;

        sunrise_loop = 1;
      }

      // Gradually move each color component towards target
      if (interpolatedR < targetR) interpolatedR += targetStepR;
      else if (interpolatedR > targetR) interpolatedR -= targetStepR;

      if (interpolatedG < targetG) interpolatedG += targetStepG;
      else if (interpolatedG > targetG) interpolatedG -= targetStepG;

      if (interpolatedB < targetB) interpolatedB += targetStepB;
      else if (interpolatedB > targetB) interpolatedB -= targetStepB;

      // Convert to integers and set color
      r = int(interpolatedR);
      g = int(interpolatedG);
      b = int(interpolatedB);

      currentColor = matrix.Color(r, g, b);
      terminalOutput("r:" + String(r) + "g:" + String(g) + "b:" + String(b) + " | Target: " + String(targetR) + "," + String(targetG) + "," + String(targetB));
      matrix.setTextColor(currentColor);
    } else {
      if (sunrise_loop == 0) {
        //SUNRISE LOOP HAS NOT RUN
        RGB565TORGB(settings.nightColor, r, g, b);
        currentColor = matrix.Color(r, g, b);
        matrix.setTextColor(currentColor);
      } else {
        sunrise_loop = 1;
        RGB565TORGB(settings.mainColor, r, g, b);
        currentColor = matrix.Color(r, g, b);
        matrix.setTextColor(currentColor);
      }
    }


    matrix.fillScreen(0);
    //draw clock
    if (clockHour < 10) {
      cursor = 0;
      matrix.setCursor(cursor, 7);
      matrix.print(String(clockHour % 10));
      cursor = cursor + 2;
    } else {
      cursor = 0;
      matrix.setCursor(cursor, 7);
      matrix.print(String(clockHour / 10));

      cursor = cursor + 3;
      cursor++;
      matrix.setCursor(cursor, 7);
      matrix.print(String(clockHour % 10));
      cursor = cursor + 2;
    }
    cursor = cursor + 3;
    cursor++;
    matrix.setCursor(cursor, 7);
    matrix.print(String(clockMin / 10));

    cursor = cursor + 3;
    cursor++;
    matrix.setCursor(cursor, 7);
    matrix.print(String(clockMin % 10));

    matrix.show();
  }
}

void minimalisticTick(short int x_loc, short int y_loc_led1, short int y_loc_led2) {
  uint16_t color = matrix.Color(int(r), int(g), int(b));
  short int correction;
  if ((millis() - tmp_millis) > 500 && (millis() - tmp_millis) <= 1000) {
    color = matrix.Color(int(r), int(g), int(b));
  } else if ((millis() - tmp_millis) < 500 && (millis() - tmp_millis) <= 1000) {
    color = 0;
  } else if (((millis() - tmp_millis) > 1000) && ((millis() - tmp_millis) < 1999)) {
    //in this case tmp_millis might be misaligned with the seconds, so this tries to correct it.
    color = 0;
    correction = 1000 - (millis() - tmp_millis);
    tmp_millis = millis() - correction;
  } else if ((millis() - tmp_millis) >= 2000) {
    color = 0;
    //has drifted too far
    // Clock_Sec = country.second();
    tmp_millis = millis() + country.ms();  //correct it with ms from eztime
  }
  matrix.drawPixel(x_loc, y_loc_led1, color);
  matrix.drawPixel(x_loc, y_loc_led2, color);
  matrix.show();
}

void test() {
  matrix.fillScreen(matrix.Color(255, 255, 255));
  matrix.show();
  delay(1000);
}

void handleButton() {
  unsigned short int debounceDelay = 50;  // Debounce time in milliseconds

  int reading = digitalRead(buttonPin);
  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }




  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading != buttonState) {
      buttonState = reading;
      if (buttonState == LOW) {
        // // The button is pressed, do something
        // r = random(0, 255);
        // g = random(0, 255);
        // b = random(0, 255);
        // fadeStepR= double(r) / 25;
        // fadeStepG = double(g) / 25;
        // fadeStepB = double(b) / 25;
        // matrix.setTextColor(matrix.Color(r, g, b));
        // terminalOutput("Button Pressed!");

        getWeather();
        fadeOut(100);
        matrix.fillScreen(0);
        matrix.setBrightness(settings.maxbrightness);
        matrix.setFont(&Normal);
        scrollText(weather_str, 50);
        matrix.setFont(&square_big_font);
        fadeOut(100);
        loop();
      }
    }
  }
  lastButtonState = reading;
}

void syncSecondsWithRTC() {

  if (LastRTCSyncTime != (country.hour() * 100 + country.minute())) {  //prevent exagerated sync

    DateTime rtcTime = rtc.now();

    if (rtcTime.hour() != country.hour()) {
      terminalOutput("rtc hour different than internal hour");
      DateTime rtcTime = rtc.now();
      rtc.adjust(DateTime(country.year(), country.month(), country.day(), country.hour(), country.minute(), rtcTime.second()));
      terminalOutput("rtc hour adjusted");
      rtcTime = rtc.now();
    }

    unsigned long unixTime = rtcTime.unixtime();  // Get Unix timestamp from RTC
    country.setTime(unixTime);                    // Sync ezTime to RTC time
    LastRTCSyncTime = country.hour() * 100 + country.minute();
    terminalOutput("time synced with rtc");
  }
}


void fadeOut(int duration) {  //duration in miliseconds
  int brightness = matrix.getBrightness();
  int step = round((float)duration / brightness);
  for (int i = brightness; i > 0; i -= step) {
    matrix.setBrightness(i);  //used the normal setBrightness directly because the value should never be above max brightness anyway
    matrix.show();
    //Serial.println(i);
    //Serial.println(matrix.getBrightness());
  }
  matrix.setBrightness(0);  //set to 0
  matrix.show();
  //Serial.println(matrix.getBrightness());
}

void fadeIn(int speed, int targetBrightness) {
  int brightness = matrix.getBrightness();
  (targetBrightness > settings.maxbrightness) ? targetBrightness : settings.maxbrightness;
  for (int i = brightness; i < targetBrightness; i++) {
    brightness = matrix.getBrightness();
    matrix.setBrightness(i);
    matrix.show();
    delay(speed);
  }
}

uint8_t utf8Ascii(uint8_t ascii) {
  static uint8_t cPrev;     // last characacter
  static uint8_t cPrePrev;  // penultimate character
  uint8_t c = '\0';

  if (ascii < 0x7f) {
    cPrev = '\0';
    cPrePrev = '\0';
    c = ascii;
  } else {
    switch (cPrev) {
      case 0xC2: c = ascii; break;
      case 0xC3: c = ascii | 0xC0; break;
      case 0x82:
        if (ascii == 0xAC) c = 0x80;  // Euro symbol special case
    }
    switch (cPrePrev) {
      case 0xE2:
        if (cPrev == 0x80 & ascii == 0xA6) c = 133;
        if (cPrev == 0x80 & ascii == 0x93) c = 150;
        break;
    }
    cPrePrev = cPrev;  // save penultimate character
    cPrev = ascii;     // save last character
  }
  return (c);
}


void convert(const char* source, char* destination)  // picks every character from char *source,
{                                                    // converts it and writes it into char* destination
  int k = 0;
  char c;
  for (int i = 0; i < strlen(source); i++) {
    c = utf8Ascii(source[i]);
    if (c != 0)
      destination[k++] = c;
  }
  destination[k] = 0;
}

void scrollText(String text, int speed) {
  int16_t x1, y1;
  uint16_t w, h;

  // Convert String to const char* for use with getTextBounds
  const char* textChar = text.c_str();

  // Create a buffer to hold the converted text
  char convertedText[100];           // Adjust size based on your needs
  convert(textChar, convertedText);  // Convert the text

  // Get text bounds for scrolling calculations
  matrix.getTextBounds(convertedText, 0, 0, &x1, &y1, &w, &h);

  // Calculate the endpoint where the last character is fully displayed
  int16_t endPosition = -w + matrix.width();

  // Scroll the converted text across the screen
  for (int16_t x = matrix.width(); x >= endPosition; x--) {
    matrix.fillScreen(0);         // Clear the screen
    matrix.setCursor(x, 7);       // Set the text position (7 is usually good for an 8-pixel high display)
    matrix.print(convertedText);  // Print the converted text
    matrix.show();                // Update the display
    delay(speed);                 // Adjust the delay for scrolling speed
  }

  // Delay between each scroll
  delay(speed * 10);  // Adjust this multiplier to change the pause between scrolls
}

void serverSaveColor() {
  if (server.hasArg("Base_color") && server.hasArg("Night_color")) {
    String Base_color = server.arg("Base_color");
    String Night_color = server.arg("Night_color");

    int server_r = (int)strtol(&Base_color[1], NULL, 16) >> 16 & 0xFF;
    int server_g = (int)strtol(&Base_color[1], NULL, 16) >> 8 & 0xFF;
    int server_b = (int)strtol(&Base_color[1], NULL, 16) & 0xFF;

    r = server_r;
    g = server_g;
    b = server_b;
    maxChannelValue = max(max(r, g), b);  // Use max function directly for max RGB
    if (settings.randomColor) {
      targetR = targetB = targetG = 0;
    }

    interpolatedR = 0;  //this is used for the random color assignment
    interpolatedG = 0;
    interpolatedB = 0;
    fadeStepR= double(r) / 25;
    fadeStepG = double(g) / 25;
    fadeStepB = double(b) / 25;
    settings.mainColor = matrix.Color(r, g, b);

    server_r = (int)strtol(&Night_color[1], NULL, 16) >> 16 & 0xFF;
    server_g = (int)strtol(&Night_color[1], NULL, 16) >> 8 & 0xFF;
    server_b = (int)strtol(&Night_color[1], NULL, 16) & 0xFF;

    settings.nightColor = matrix.Color(server_r, server_g, server_b);

    fadeOut(200);

    server.send(200, "text/plain", "Color set successfully");
    loop();

  } else {
    server.send(400, "text/plain", "Missing parameters");
  }
}


void testColor() {
  String outText;
  if (server.hasArg("type") && server.hasArg("color")) {
    if (server.arg("type") == "base") {
      screenClockHour = 88;
      screenClockMin = 88;
      String color = server.arg("color");
      //RGB565TORGB(color,server_r,server_g,server_b);

      int server_r = (int)strtol(&color[1], NULL, 16) >> 16 & 0xFF;
      int server_g = (int)strtol(&color[1], NULL, 16) >> 8 & 0xFF;
      int server_b = (int)strtol(&color[1], NULL, 16) & 0xFF;

      unsigned long int time_to_reach = millis() + 5000;

      uint16_t test_color = matrix.Color(server_r, server_g, server_b);

      fadeOut(500);
      matrix.setTextColor(test_color);
      matrix.setBrightness(175);
      while (millis() < time_to_reach) {
        matrix.setFont(&square_big_font);
        matrix.fillScreen(0);
        outText = "88:88";
        matrix.setCursor(4, 7);
        matrix.print(outText);
        matrix.show();
      }
      modeInitialized = false;
      loop();
    } else if (server.arg("type") == "night") {
      screenClockHour = 88;
      screenClockMin = 88;
      String color = server.arg("color");

      int server_r = (int)strtol(&color[1], NULL, 16) >> 16 & 0xFF;
      int server_g = (int)strtol(&color[1], NULL, 16) >> 8 & 0xFF;
      int server_b = (int)strtol(&color[1], NULL, 16) & 0xFF;

      unsigned long int time_to_reach = millis() + 5000;

      uint16_t test_color = matrix.Color(server_r, server_g, server_b);

      fadeOut(500);
      matrix.setTextColor(test_color);
      matrix.setBrightness(175);
      while (millis() < time_to_reach) {
        matrix.setFont(&Mini_Font);
        matrix.fillScreen(0);
        outText = "88:88";
        matrix.setCursor(0, 7);
        matrix.print(outText);
        matrix.show();
      }
      modeInitialized = false;
      loop();
    }
    server.send(200, "text/plain", "Color set successfully");
  } else {
    server.send(400, "text/plain", "Missing parameters");
  }
}


void replacePlaceholder(char* str, const char* placeholder, const char* value) {
  char* ptr = strstr(str, placeholder);
  if (ptr != NULL) {
    strcpy(ptr, value);
  }
}

void RGB565TORGB(uint16_t rgb565Color, uint8_t& red, uint8_t& green, uint8_t& blue) {
  // Extracting individual color components
  uint8_t r = (rgb565Color >> 11) & 0x1F;  // Extracting 5 bits for red (R:5)
  uint8_t g = (rgb565Color >> 5) & 0x3F;   // Extracting 6 bits for green (G:6)
  uint8_t b = rgb565Color & 0x1F;          // Extracting 5 bits for blue (B:5)

  // Scaling components to 8-bit range (0-255)
  red = (r * 255) / 31;    // 31 is the maximum value for 5 bits
  green = (g * 255) / 63;  // 63 is the maximum value for 6 bits
  blue = (b * 255) / 31;   // 31 is the maximum value for 5 bits
}

uint32_t RGB565toRGB888(uint16_t RGB565) {
  uint8_t r8, g8, b8;

  // Extracting individual RGB565 components from RGB565
  r8 = ((((RGB565 >> 11) & 0x1F) * 527) + 23) >> 6;
  g8 = ((((RGB565 >> 5) & 0x3F) * 259) + 33) >> 6;
  b8 = (((RGB565 & 0x1F) * 527) + 23) >> 6;

  // Combining components into RGB888 format
  uint32_t RGB888 = ((uint32_t)r8 << 16) | ((uint32_t)g8 << 8) | b8;

  return RGB888;
}

bool isValidColor(uint16_t color) {
  // Extract red, green, and blue components
  uint8_t red = (color >> 11) & 0x1F;
  uint8_t green = (color >> 5) & 0x3F;
  uint8_t blue = color & 0x1F;

  // Check if components are within range
  return (red <= 0x1F && green <= 0x3F && blue <= 0x1F);
}

void startWebServers() {
  server.on("/", HTTP_GET, []() {
    unsigned long currentTime = millis();
    unsigned long days = currentTime / 86400000;  // 1 day = 24 * 60 * 60 * 1000 milliseconds
    currentTime %= 86400000;                      // Remaining milliseconds after extracting days
    byte hours = currentTime / 3600000;           // 1 hour = 60 * 60 * 1000 milliseconds
    currentTime %= 3600000;                       // Remaining milliseconds after extracting hours
    byte minutes = currentTime / 60000;           // 1 minute = 60 * 1000 milliseconds
    currentTime %= 60000;                         // Remaining milliseconds after extracting minutes
    byte seconds = currentTime / 1000;            // 1 second = 1000 milliseconds


    sensors.requestTemperatures();
    if (tempDataHour.size() == 0) {
      tempDataHour.push(country.hour());
      tempDataMin.push(country.minute());
      tempDataSensor.push(sensors.getTempCByIndex(0));
    }
    DateTime now = rtc.now();

    String formattedTimeString, formattedTempDataSensor1, formattedTerminalOutput;

    for (size_t i = 0; i < tempDataHour.size(); ++i) {
      // Concatenate hour and minute with a colon
      formattedTimeString += (tempDataHour[i] < 10 ? "0" + String(tempDataHour[i]) : String(tempDataHour[i])) + ":" + (tempDataMin[i] < 10 ? "0" + String(tempDataMin[i]) : String(tempDataMin[i]));
      formattedTempDataSensor1 += String(tempDataSensor[i]);
      // Add comma if not the last element
      if (i != tempDataHour.size() - 1) {
        formattedTimeString += ",";
        formattedTempDataSensor1 += ",";
      }
    }
    for (size_t i = 0; i < terminalBuffer.size(); ++i) {
      formattedTerminalOutput += terminalBuffer[i] + "\n";
    }
    String page = String(MAIN_page);
    page.replace("{{WiFi}}", WiFi.SSID());
    page.replace("{{IP}}", WiFi.localIP().toString());
    page.replace("{{MAX_BRIGHTNESS}}", String(settings.maxbrightness));
    page.replace("{{POSIX}}", String(C_POSIX));
    page.replace("{{NIGHTCLOCKSTART}}", String(settings.nightModeStartHour < 10 ? "0" + String(settings.nightModeStartHour) : String(settings.nightModeStartHour)) + ":" + String(settings.nightModeStartMin < 10 ? "0" + String(settings.nightModeStartMin) : String(settings.nightModeStartMin)));
    page.replace("{{NIGHTCLOCKEND}}", String(settings.nightModeEndHour < 10 ? "0" + String(settings.nightModeEndHour) : String(settings.nightModeEndHour)) + ":" + String(settings.nightModeEndMin < 10 ? "0" + String(settings.nightModeEndMin) : String(settings.nightModeEndMin)));
    page.replace("{{MAIN_COLOR}}", "#" + String(RGB565toRGB888(settings.mainColor), HEX));
    page.replace("{{NIGHT_COLOR}}", "#" + String(RGB565toRGB888(settings.nightColor), HEX));
    page.replace("{{FIRMWAREVERSION}}", "V:" + String(firmwareVersion));
    page.replace("{{CURRENTHOUR}}", String(clockHour) + ":" + String(clockMin));
    page.replace("{{ezTime}}", country.dateTime());
    page.replace("{{UPTIME}}", String(days) + "d " + String(hours) + "h " + String(minutes) + "m " + String(seconds) + "s");
    page.replace("{{RTCCHIP}}", String(now.day()) + "/" + String(now.month()) + "/" + String(now.year()) + "|" + String(now.hour()) + ":" + String(now.minute()) + ":" + String(now.second()));
    page.replace("{{SENSOR1VALUE}}", String(sensors.getTempCByIndex(0)) + "ºC");
    page.replace("{{SENSOR2VALUE}}", String(rtc.getTemperature()) + "ºC");
    page.replace("{{X_AXIS}}", formattedTimeString);
    page.replace("{{Y_SENSOR1DATA}}", formattedTempDataSensor1);

    page.replace("{{terminalOutput}}", formattedTerminalOutput);
    page.replace("{{PLACE}}", settings.city);
    if (settings.randomColor) {
      page.replace("{{RANDOMCOLORCHECKBOX}}", "checked ");
    } else {
      page.replace("{{RANDOMCOLORCHECKBOX}}", " ");
    }
    if (settings.weather) {
      page.replace("{{WEATHERCHECKBOX}}", "checked ");
    } else {
      page.replace("{{WEATHERCHECKBOX}}", " ");
    }
    server.send(200, "text/html", page);
  });

  server.on("/SaveNewSettings", HTTP_POST, handleSaveNewSettings);

  server.on("/JsonSensorData", HTTP_GET, []() {
    if (tempDataHour.size() == 0) {
      sensors.requestTemperatures();
      tempDataHour.push(country.hour());
      tempDataMin.push(country.minute());
      tempDataSensor.push(sensors.getTempCByIndex(0));
    }
    server.send(200, "application/json", "{\"Temperature\": " + String(tempDataSensor.last()) + "}");
  });

  server.on("/Sensor", HTTP_GET, []() {
    if (tempDataHour.size() == 0) {
      DateTime now = rtc.now();
      sensors.requestTemperatures();
      tempDataHour.push(country.hour());
      tempDataMin.push(country.minute());
      tempDataSensor.push(sensors.getTempCByIndex(0));
    }


    String formattedTimeString, formattedTempDataSensor1, formattedTempDataSensor2;


    for (size_t i = 0; i < tempDataHour.size(); ++i) {
      formattedTimeString +=
        (tempDataHour[i] < 10 ? "0" + String(tempDataHour[i]) : String(tempDataHour[i])) + ":" + (tempDataMin[i] < 10 ? "0" + String(tempDataMin[i]) : String(tempDataMin[i]));

      formattedTempDataSensor1 += String(tempDataSensor[i]);

      if (i != tempDataHour.size() - 1) {
        formattedTimeString += ",";
        formattedTempDataSensor1 += ",";
      }
    }

    String page = String(TempPage);
    page.replace("{{CUSTOM_TEXT}}", "V:" + String(firmwareVersion) + "<br>João Fernandes");
    page.replace("{{X_AXIS}}", formattedTimeString);
    page.replace("{{Y_SENSOR1DATA}}", formattedTempDataSensor1);
    server.send(200, "text/html", page);
  });

  server.on("/Color", HTTP_GET, []() {
    String page = String(ColorPickerPage);
    float pageMaxBrightness = (float)settings.maxbrightness * 0.39;  // 100/255 value approximated
    page.replace("{{CUSTOM_TEXT}}", "V:" + String(firmwareVersion) + "<br>João Fernandes");
    page.replace("{{MAX_BRIGHTNESS}}", String(pageMaxBrightness));
    page.replace("{{MAIN_COLOR}}", "#" + String(RGB565toRGB888(settings.mainColor), HEX));
    page.replace("{{NIGHT_COLOR}}", "#" + String(RGB565toRGB888(settings.nightColor), HEX));
    server.send(200, "text/html", page);
  });

  server.on("/ESPReset", HTTP_POST, []() {
    ESP.restart();
  });

  server.on("/iro.min.js", HTTP_GET, []() {
    server.send_P(200, "text/javascript", iro_js);
  });

  server.on("/SaveColor", HTTP_POST, serverSaveColor);
  server.on("/testColor", HTTP_POST, testColor);

  server.on("/TempRequest", HTTP_POST, []() {
    sensors.requestTemperatures();
    tempDataHour.push(country.hour());
    tempDataMin.push(country.minute());
    tempDataSensor.push(sensors.getTempCByIndex(0));

    server.send(200, "text/plain", "temperatures requested sucessfully");
  });



  server.begin();
}
//if (last_weather_update_hour > 24 || last_weather_update_hour != clockHour) {
void getWeather() {
  bool weather = true;

  HTTPClient http;  // Object of class HTTPClient
  http.useHTTP10(true);

  String url = "http://api.weatherapi.com/v1/forecast.json?key=" + APIKEY + "&q=" + settings.city + "&days=1&aqi=no&alerts=no&lang=" + settings.language;
  if (clockHour == 23) {
    url += "&hour=23";
  } else {
    url += "&hour=" + String(clockHour + 1);
  }
  if (last_weather_update_hour > 24 || last_weather_update_hour != clockHour) {  //only update if the last update was more than 1 hour ago or it was never run
    http.begin(wifiClient, url);                                                 // Begin HTTP request
    int httpCode = http.GET();                                                   // Get the response code

    if (httpCode <= 0) {
      weather = false;
    } else {
      Stream& input = http.getStream();

      // Create a filter for the JSON data we want to extract
      StaticJsonDocument<160> filter;
      JsonObject filter_forecast = filter["forecast"]["forecastday"][0]["hour"][0].to<JsonObject>();
      filter_forecast["temp_c"] = true;
      filter_forecast["time"] = true;
      filter_forecast["humidity"] = true;
      filter_forecast["condition"]["text"] = true;

      // Filter for astro data (new part)
      JsonObject filter_astro = filter["forecast"]["forecastday"][0]["astro"].to<JsonObject>();
      filter_astro["sunrise"] = true;
      //filter_astro["sunset"] = true;

      // Create a document to hold the data
      StaticJsonDocument<512> doc;
      ReadBufferingStream bufferedFile(input, 64);

      // Deserialize the JSON response with a filter applied
      DeserializationError error = deserializeJson(doc, bufferedFile, DeserializationOption::Filter(filter));

      if (error) {
        terminalOutput("deserializeJson() failed: ");
        terminalOutput(error.f_str());
        return;
      }

      // Access the values in the filtered document
      JsonObject forecast_forecastday = doc["forecast"]["forecastday"][0]["hour"][0];
      const char* forecast_time = forecast_forecastday["time"];                    // e.g. "2021-09-24 23:00"
      float forecast_temp = forecast_forecastday["temp_c"];                  // e.g. 16.1
      int forecast_hum = forecast_forecastday["humidity"];                 // e.g. 80
      const char* forecast_condition = forecast_forecastday["condition"]["text"];  // e.g. "Clear"

      // Prepare the weather string
      short int tmp_t = round(forecast_temp);
      weather_str = String(forecast_condition) + " | " + forecast_hum + "%" + " | " + String(tmp_t) + "ºC";

      // Access the values in the astro document
      JsonObject astro = doc["forecast"]["forecastday"][0]["astro"];
      const char* sunrise = astro["sunrise"];  // e.g. "07:44 AM"
      //const char* sunset = astro["sunset"];

      // Convert sunrise to 24-hour format
      String sunriseStr(sunrise);                      // Convert the C-string to String for easier manipulation
      sunrise_h = sunriseStr.substring(0, 2).toInt();  // Extract hour part
      sunrise_m = sunriseStr.substring(3, 5).toInt();  // Extract minute part
      String ampm = sunriseStr.substring(6, 8);        // Extract AM/PM part

      // Adjust hour based on AM/PM
      if (ampm == "PM" && sunrise_h != 12) {
        sunrise_h += 12;  // Convert PM to 24-hour format
      }
      if (ampm == "AM" && sunrise_h == 12) {
        sunrise_h = 0;  // Convert midnight (12 AM) to 00 hours
      }

      // Print the results
      terminalOutput(forecast_time);
      terminalOutput(weather_str);
      terminalOutput("Sunrise:" + String(sunrise_h) + ":" + String(sunrise_m));
    }
    last_weather_update_hour = country.hour();
    http.end();  // Close the connection
  }
}


void handleSaveNewSettings() {
  if (server.hasArg("maxbrightness")) {
    settings.maxbrightness = (uint8_t)server.arg("maxbrightness").toInt();
  }

  if (server.hasArg("mainColor")) {
    settings.mainColor = (uint16_t)strtoul(server.arg("mainColor").c_str(), nullptr, 0);
  }

  if (server.hasArg("nightColor")) {
    settings.nightColor = (uint16_t)strtoul(server.arg("nightColor").c_str(), nullptr, 0);
  }

  if (server.hasArg("place")) {
    strlcpy(settings.city, server.arg("place").c_str(), sizeof(settings.city));
  }

  if (server.hasArg("language")) {
    strlcpy(settings.language, server.arg("language").c_str(), sizeof(settings.language));
  }

  settings.weather = server.hasArg("weather");
  settings.randomColor = server.hasArg("randomcolor");

  if (server.hasArg("start")) {
    String start = server.arg("start");
    if (start.length() >= 5) {
      settings.nightModeStartHour = start.substring(0, 2).toInt();
      settings.nightModeStartMin = start.substring(3, 5).toInt();
    }
  }

  if (server.hasArg("end")) {
    String end = server.arg("end");
    if (end.length() >= 5) {
      settings.nightModeEndHour = end.substring(0, 2).toInt();
      settings.nightModeEndMin = end.substring(3, 5).toInt();
    }
  }

  saveSettings(settings);

  terminalOutput("Settings updated from Web UI and saved to NVS");
  server.send(200, "text/plain", "OK");
}


void terminalOutput(String text) {
  text = String(clockHour < 10 ? "0" : "") + clockHour + ":" + (clockMin < 10 ? "0" : "") + clockMin + " : " + text;
  Serial.println(text);
  terminalBuffer.push(text);
}

void setDefaultSettings(AppSettings& s) {
  s.maxbrightness = 175;

  s.mainColor = 0xad75;
  s.nightColor = 0x8c71;

  s.randomColor = true;
  s.weather = true;

  strlcpy(s.city, "Aveiro,Portugal", sizeof(s.city));
  strlcpy(s.language, "en", sizeof(s.language));

  s.nightModeStartHour = 23;
  s.nightModeStartMin = 0;
  s.nightModeEndHour = 10;
  s.nightModeEndMin = 30;
}

void saveSettings(const AppSettings& s) {
  preferences.begin("matrixcfg", false);

  preferences.putBool("hasInit", true);

  preferences.putUChar("maxbright", s.maxbrightness);
  preferences.putUShort("mainColor", s.mainColor);
  preferences.putUShort("nightColor", s.nightColor);

  preferences.putBool("randColor", s.randomColor);
  preferences.putBool("weather", s.weather);

  preferences.putString("city", s.city);
  preferences.putString("lang", s.language);

  preferences.putUChar("nm_st_h", s.nightModeStartHour);
  preferences.putUChar("nm_st_m", s.nightModeStartMin);
  preferences.putUChar("nm_en_h", s.nightModeEndHour);
  preferences.putUChar("nm_en_m", s.nightModeEndMin);

  preferences.end();
}

void loadSettings(AppSettings& s) {
  preferences.begin("matrixcfg", true);

  bool hasInit = preferences.getBool("hasInit", false);

  if (!hasInit) {
    preferences.end();
    setDefaultSettings(s);
    saveSettings(s);
    return;
  }

  s.maxbrightness = preferences.getUChar("maxbright", 175);
  s.mainColor = preferences.getUShort("mainColor", 0xad75);
  s.nightColor = preferences.getUShort("nightColor", 0x8c71);

  s.randomColor = preferences.getBool("randColor", true);
  s.weather = preferences.getBool("weather", true);

  String cidadeTmp = preferences.getString("city", "Aveiro,Portugal");
  String langTmp = preferences.getString("lang", "en");

  strlcpy(s.city, cidadeTmp.c_str(), sizeof(s.city));
  strlcpy(s.language, langTmp.c_str(), sizeof(s.language));

  s.nightModeStartHour = preferences.getUChar("nm_st_h", 23);
  s.nightModeStartMin = preferences.getUChar("nm_st_m", 0);
  s.nightModeEndHour = preferences.getUChar("nm_en_h", 10);
  s.nightModeEndMin = preferences.getUChar("nm_en_m", 30);

  preferences.end();
}