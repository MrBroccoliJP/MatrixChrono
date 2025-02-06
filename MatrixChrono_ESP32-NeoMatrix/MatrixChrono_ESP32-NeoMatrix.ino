#include <WiFiManager.h>  // https://github.com/tzapu/WiFiManager
#include <WebServer.h>
#include <WiFiClient.h>
WiFiClient wifiClient;
#include <CircularBuffer.hpp>
#include <EEPROM.h>
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

RTC_DS3231 rtc;
#define PT_POSIX "WET0WEST,M3.5.0/1,M10.5.0/2"

char* C_POSIX = PT_POSIX;

const char* Version = "5.36";


Adafruit_NeoMatrix matrix = Adafruit_NeoMatrix(8, 8, 4, 1, PIN,
                                               NEO_TILE_TOP + NEO_TILE_LEFT + NEO_TILE_ROWS + NEO_TILE_PROGRESSIVE + NEO_MATRIX_TOP + NEO_MATRIX_LEFT + NEO_MATRIX_ROWS + NEO_TILE_PROGRESSIVE,
                                               NEO_GRB + NEO_KHZ800);
#include "icons.h"  //this needs to be after the matrix declaration since it uses matrix stuff

Timezone country;
const int oneWireBus = 32;
OneWire oneWire(oneWireBus);
DallasTemperature sensors(&oneWire);
WebServer server(80);

//AsyncWebServer server(80);
bool wifiportal;  //used to see if the wifi is connected or not.

uint8_t r = 0;             //main red color
uint8_t g = 0;             //main green color
uint8_t b = 0;             //main blue color
uint8_t red, green, blue;  //USED FOR THE RGB565TORGB FUNCTION
bool randomColor = 1;
uint8_t maxRGBValue;
uint8_t next_r, next_g, next_b;                           //used to store the max value of r,g,b in order to calculate a random color with the same "brightness"
float dec_next_r, dec_next_g, dec_next_b;                 //these values are used to calculate the random color change
float dec_next_r_calc, dec_next_g_calc, dec_next_b_calc;  //float values to temporarily calculate the values of r,g,b if random color
float r_tmp = 0;                                          //used for calculation for red in tick effect
float g_tmp = 0;                                          //used for calculation for green in tick effect
float b_tmp = 0;                                          //used for calculation for blue in tick effect
float r_dec = 0;                                          //used for calculation of the increments for red
float g_dec = 0;                                          //used for calculation of the increments for green
float b_dec = 0;                                          //used for calculation of the increments for blue

//max brightness 175
uint8_t buttonPin = 26;
uint8_t maxbrightness = 175;  //this is dependent on the psu
uint16_t MainColor = 0xad75;
uint16_t NightColor = 0x8c71;
uint16_t CurrentColor = 0;
uint8_t x = 0;

unsigned long tmp_millis = millis();  //used to store millis information
unsigned long wifi_millis = 0;
unsigned long check_wifi_millis = 0;
unsigned long int ShowTemp;
uint8_t last_tempreading;

//wEATHER VARIABLES
uint8_t last_weather_update_hour = 25;
bool weather = true;
String CIDADE = "Aveiro";
char CIDADE_c[10];
const char* forecast_time;
float forecast_temp = 0.0;
int forecast_hum = 0;
const char* forecast_condition = "null";
String weather_str;
// char weather_c[64];
String Language = "pt";
short int sunrise_h = 8;
short int sunrise_m = 00;  //set to 08:00 in case it fails to get weather and does not update it.

//static "variables"

const char* ntpServer = "pool.ntp.org";
const char* timeZone = "Europe/Lisbon";

const long gmtOffset_sec = 0;
const int daylightOffset_sec = 3600;

float temperatureC;
//BUTTON DEBOUNCE
bool buttonState = HIGH;             // Current state of the button
bool lastButtonState = HIGH;         // Previous state of the button
unsigned long lastDebounceTime = 0;  // Last time the button state changed


String out_text;
String Clock_Hour_Str;
String Clock_Min_Str;
uint8_t Clock_Hour;
uint8_t Clock_Min;
int8_t Screen_Clock_Hour = -1;
int8_t Screen_Clock_Min = -1;
uint8_t Clock_Sec;  //only used for certain functions.
short int LastRTCSyncTime = -1;

uint8_t NightModeStartHour = 00;
uint8_t NightModeStartMin = 00;
uint8_t NightModeEndHour = 11;
uint8_t NightModeEndMin = 00;
int8_t ClockSwitchFlag = -1;
bool sunrise_loop = 0;

CircularBuffer<uint8_t, 100> TempDataTimeHour;
CircularBuffer<uint8_t, 100> TempDataTimeMin;
CircularBuffer<float, 100> TempDataSensor1;  //Actual sensor
CircularBuffer<String, 20> TerminalBuffer;   

bool wifi_connected = 0;
WiFiManager wm;

void setup() {

  pinMode(buttonPin, INPUT_PULLUP);
  Serial.begin(115200);

  //initialization
  matrix.begin();
  matrix.setTextWrap(false);
  matrix.setBrightness(maxbrightness);
  matrix.fillScreen(0);
  matrix.setFont(&Normal);
  matrix.setTextSize(1);
  out_text = "V" + String(Version);
  matrix.setCursor(0, 7);
  matrix.print(out_text);
  matrix.show();
  delay(1000);
  fadeOut(500);  //fadeout in milliseconds
  matrix.fillScreen(0);

  WiFi.mode(WIFI_STA);  // explicitly set mode, esp defaults to STA+AP
  WiFi.setSleep(WIFI_PS_NONE);
  wm.setConfigPortalBlocking(false);  // Set configuration portal to non-blocking mode
  wm.setConfigPortalTimeout(120);     // Set configuration portal timeout to 60 seconds

  // Attempt to connect using saved credentials
  wifiportal = wm.autoConnect("NEOCLOCK_AP");

  //delay for wifi connection?
  matrix.setBrightness(maxbrightness);
  // out_text = "WiFi Connecting";
  // matrix.setCursor(0, 8);
  // matrix.print(out_text);
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
    TerminalOutput("Connected to WiFi using saved credentials");
    //Serial.println("Connected to WiFi using saved credentials");

    wifi_connected = 1;
  } else {
    wifi_connected = 0;

    //Serial.println("Failed to connect to WiFi using saved credentials");
    TerminalOutput("Failed to connect to WiFi using saved credentials");
    TerminalOutput("Configuration portal running");
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
    matrix.setBrightness(maxbrightness);
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
  matrix.setBrightness(maxbrightness);
  scrollText(WiFi.localIP().toString(), 50);

  //Serial.println("Setup1" + wifi_connected);
  TerminalOutput("Setup Flag1 - wifiConnected:" + String(wifi_connected));


  rtc.begin();
  country.setPosix(C_POSIX);


  if (wifi_connected) {
    //if the wifi is connected, set time from WEB
    waitForSync();
    setInterval(0);  //disable the sync timer from eztime~THIS NEEDS TO BE AFTER THE Sync for some reason

    TerminalOutput("time updated: " + String(lastNtpUpdateTime()));
    Clock_Sec = country.second();
    while (Clock_Sec == country.second()) {  //THIS IS USED TO SYNC THE SECONDS OF THE RTC WITH THE SECONDS FROM EZTIME
      yield();
    }
    rtc.adjust(DateTime(country.year(), country.month(), country.day(), country.hour(), country.minute(), country.second()));
    LastRTCSyncTime = country.hour() * 1000 + country.minute();

  } else {
    DateTime now = rtc.now();
    //if the wifi is not connected set time from rtc
    country.setTime(now.hour(), now.minute(), now.second(), now.day(), now.month(), now.year());
    TerminalOutput("time set from rtc clock since wifi not connected");
  }


  //if(wifi_connected){
  Start_Web_Servers();
  //}

  //READ EEPROM STORAGE
  EEPROM.begin(4);

  EEPROM.get(0, MainColor);
  EEPROM.get(2, NightColor);
  if (!isValidColor(MainColor)) {
    MainColor = 0xad75;
  }
  if (!isValidColor(NightColor)) {
    NightColor = 0x8c71;
  }


  // r = random(0, 255);
  // g = random(0, 255);
  // b = random(0, 255);
  RGB565TORGB(MainColor, r, g, b);
  r_tmp = r;
  g_tmp = g;
  b_tmp = b;
  matrix.setTextColor(matrix.Color(r, g, b));
  r_dec = double(r) / 25;
  g_dec = double(g) / 23;  //green is a more powerful color
  b_dec = double(b) / 24;
  TerminalOutput(String(r_dec) + "|" + String(g_dec) + "|" + String(b_dec));

  sensors.requestTemperatures();
  DateTime now = rtc.now();

  TerminalOutput("Setup end");
}



void loop() {
  // Always process WiFi Manager, OTA updates, and the web server

  wm.process();
  handleButton();
  ArduinoOTA.handle();

  // Check if WiFi is connected, if not, try to reconnect, wifi_millis is the time between checks
  if (WiFi.status() != WL_CONNECTED && (millis() - wifi_millis > 120000 || wifi_millis == 0)) {
    wifi_connected = 0;
    server.close();  // Close all active connections
    TerminalOutput("WiFi not connected. Attempting to reconnect...");

    //signal in the screen that wifi is not connected
    matrix.drawPixel(31, 0, 0xf800);
    // Try to reconnect using autoConnect() (non-blocking)
    wm.setConfigPortalTimeout(120);
    wifiportal = wm.autoConnect("NEOCLOCK_AP");
    TerminalOutput("Web server started.");
    wifi_millis = millis();
  }

  if (WiFi.status() == WL_CONNECTED && wifi_connected == 0) {
    //Start_Web_Servers();

    if (!(wifiClient.connect(WiFi.localIP(), 80)) && millis() - wifi_millis > 60000) {  //this attempts to make a connection to the own esp32 webserver, if it can't connect it continues saying the wifi is not connected.
      wifi_connected = 0;
      server.close();  // Close all active connections
      TerminalOutput("stoped web pages after wifi connection");
      Start_Web_Servers();  // restart webpages
      TerminalOutput("started web pages after wifi connection");
      wifi_millis = millis();
    } else {
      wifi_connected = 1;
    }

    matrix.drawPixel(31, 0, 0x0000);
  } else if (WiFi.status() == WL_CONNECTED && wifi_connected == 1) {
    server.handleClient();

    if (!(wifiClient.connect(WiFi.localIP(), 80)) && millis() - wifi_millis > 60000) {  //this attempts to make a connection to the own esp32 webserver, if it can't connect it continues saying the wifi is not connected.
      wifi_connected = 0;
      TerminalOutput("pages are not being served");
      server.close();       // Close all active connections
      Start_Web_Servers();  // restart webpages
      TerminalOutput("restarted webpages");
    }
  }


  // Get the current time (you can cache it to avoid repeated calls)
  int currentTime = country.hour() * 100 + country.minute();
  int nightEndTime = NightModeEndHour * 100 + NightModeEndMin;

  // Determine if we are in night mode or normal mode
  if (currentTime >= nightEndTime) {
    if (ClockSwitchFlag != 1) {
      // Normal mode initialization
      sunrise_loop = 0;
      CurrentColor = MainColor;  //the MainColor property is always the starting value of the normal clock mode, even when the randomColor flag is set
      matrix.setFont(&square_big_font);
      matrix.setTextColor(CurrentColor);  //never change the values of MainColor or NightColor (PROTECTED VALUES)

      RGB565TORGB(MainColor, r, g, b);  //this sets the r,g,b values based on the MainColor property
      r_dec = double(r) / 25;
      g_dec = double(g) / 23;
      b_dec = double(b) / 24;

      maxRGBValue = max(max(r, g), b);  // Use max function directly for max RGB
      ClockSwitchFlag = 1;
    }
    // Normal Clock display
    Normal_Clock();

  } else {
    if (ClockSwitchFlag != 0) {
      // Night mode initialization
      CurrentColor = NightColor;
      matrix.setFont(&Mini_Font);
      matrix.setTextColor(CurrentColor);  //never change the values of MainColor or NightColor (PROTECTED VALUES)
      RGB565TORGB(NightColor, r, g, b);
      r_dec = double(r) / 25;
      g_dec = double(g) / 23;
      b_dec = double(b) / 24;
      ClockSwitchFlag = 0;
      //get sunrise and sunset time? using http://api.weatherapi.com
      getWeather();
    }
    // Minimalist Clock display for night mode
    minimalist_Clock();
  }
  yield();
}

void Normal_Clock() {
  short int currentBrightness = matrix.getBrightness();
  unsigned long int MillisKeep;
  while (currentBrightness != maxbrightness) {
    if (currentBrightness < maxbrightness) {
      for (int i = currentBrightness; i < maxbrightness + 1;) {
        matrix.setBrightness(i);
        refresh_normal_clock();
        fade_tick(16, 2, 6);
        currentBrightness = matrix.getBrightness();
        //Serial.println("++Brightness level: " + String(i));
        i++;
        delay(5);
      }
    } else if (currentBrightness > maxbrightness) {
      for (int i = currentBrightness; i > maxbrightness - 1;) {
        matrix.setBrightness(i);
        refresh_normal_clock();
        fade_tick(16, 2, 6);
        currentBrightness = matrix.getBrightness();
        //Serial.println("--Brightness level: " + String(i));
        i--;
        delay(5);
      }
    }
  }

  if (Screen_Clock_Hour == country.hour() && Screen_Clock_Min == country.minute()) {
    handleButton();
    ArduinoOTA.handle();
    server.handleClient();
    fade_tick(16, 2, 6);
    //matrix.show();
  } else {  //CLOCK CHANGED
    fade_tick(16, 2, 6);
    refresh_normal_clock_slideEffect();
    //temperature display every 15min starting from 5 -> 20 -> 35 -> 50
    if (Clock_Min % 15 == 5) {

      MillisKeep = millis();
      while (millis() - MillisKeep < 5000) {
        fade_tick(16, 2, 6);
        //matrix.show();
      }  //delay 5 seconds



      getWeather();
      fadeOut(100);
      matrix.fillScreen(0);
      matrix.setBrightness(maxbrightness);
      matrix.setFont(&Normal);
      scrollText(weather_str, 50);
      matrix.setFont(&square_big_font);

      //read sensor data
      sensors.requestTemperatures();
      TempDataTimeHour.push(country.hour());
      TempDataTimeMin.push(country.minute());
      TempDataSensor1.push(sensors.getTempCByIndex(0));

      //delay(5000);  //delay 5 seconds
      fadeOut(100);
      matrix.fillScreen(0);
      matrix.setBrightness(maxbrightness);
      matrix.setCursor(0, 7);
      matrix.print(String(int(TempDataSensor1.last())) + "$C");  //$ = º --> used $ to save space on the font
      matrix.show();
      Screen_Clock_Min = -1;
      Screen_Clock_Hour = -1;
      syncSecondsWithRTC();
      delay(5000);   //display temperature for 5 seconds
      fadeOut(100);  //fadeout temperature
      matrix.fillScreen(0);
      matrix.setBrightness(maxbrightness);
      refresh_normal_clock_slideEffect();
    }
  }
}

void refresh_normal_clock() {  //without any effect
  // Serial.println("refresh_normal_clock");
  short int start_cursor = 4;
  short int font_width = 5;
  font_width++;  //to account for the space between characters
  short int cursor = start_cursor;

  Clock_Hour = country.hour();
  Clock_Min = country.minute();
  Screen_Clock_Hour = Clock_Hour;
  Screen_Clock_Min = Clock_Min;
  matrix.fillScreen(0);
  matrix.setCursor(cursor, 7);
  matrix.print(String(Screen_Clock_Hour / 10));
  cursor = cursor + font_width;  // Move to the next digit position
  matrix.setCursor(cursor, 7);
  matrix.print(String(Screen_Clock_Hour % 10));
  cursor = cursor + font_width;
  cursor = cursor + 2;  // Account for spacing between hour and minute digits
  matrix.setCursor(cursor, 7);
  matrix.print(String(Screen_Clock_Min / 10));
  cursor = cursor + font_width;  // Move to the next digit position
  matrix.setCursor(cursor, 7);
  matrix.print(String(Screen_Clock_Min % 10));
  matrix.show();
}


void refresh_normal_clock_slideEffect() {
  // Serial.println("refresh_normal_clock_slideEffect");
  // Initialize variables
  short int start_cursor = 4;
  short int font_width = 5;
  font_width++;  //to account for the space between characters
  short int cursor = start_cursor;
  short int slide_timing = 5;

  // Get current time
  Clock_Hour = country.hour();
  Clock_Min = country.minute();

  if (randomColor) {  //THIS PART IS ONLY RUN IF RANDOM COLOR IS ENABLED
    if ((Screen_Clock_Hour != Clock_Hour && Screen_Clock_Hour > 0) || (next_r + next_g + next_b) <= 10) {
      if ((dec_next_r_calc + dec_next_g_calc + dec_next_b_calc) == 0) {

        dec_next_r_calc = r;
        dec_next_g_calc = g;
        dec_next_b_calc = b;
      }
      randomNextColor(maxRGBValue, 60 - Clock_Min);  //to run every hour
      TerminalOutput("next_r:" + String(next_r) + " next_g:" + String(next_g) + " next_b:" + String(next_b) + " max rgb value:" + String(maxRGBValue));
    }
    if (Screen_Clock_Min >= 0 && Screen_Clock_Min != Clock_Min) {  //to run every minute
      // Update red component
      dec_next_r_calc = (dec_next_r_calc < next_r) ? min(dec_next_r_calc + dec_next_r, float(next_r)) : max(dec_next_r_calc - dec_next_r, float(next_r));
      // Update green component
      dec_next_g_calc = (dec_next_g_calc < next_g) ? min(dec_next_g_calc + dec_next_g, float(next_g)) : max(dec_next_g_calc - dec_next_g, float(next_g));
      // Update blue component
      dec_next_b_calc = (dec_next_b_calc < next_b) ? min(dec_next_b_calc + dec_next_b, float(next_b)) : max(dec_next_b_calc - dec_next_b, float(next_b));

      r_dec = double(r) / 25;
      g_dec = double(g) / 25;
      b_dec = double(b) / 25;
      r = int(round(dec_next_r_calc));
      g = int(round(dec_next_g_calc));
      b = int(round(dec_next_b_calc));
      CurrentColor = matrix.Color(r, g, b);
      matrix.setTextColor(CurrentColor);
      TerminalOutput("r:" + String(r) + "g:" + String(g) + "b:" + String(b));
    }
  }


  // Check if the screen clock hour and minute are negative, this means the clock is not being shown on the screen so it needs a complete refresh
  if (Screen_Clock_Hour < 0 || Screen_Clock_Min < 0) {
    // Serial.println("refresh_normal_clock_slideEffect | Screen_Clock_Hour < 0 || Screen_Clock_Min < 0");
    // Loop for slide effect animation
    for (int i = 0; i < 9; i++) {
      // Print blank character
      cursor = start_cursor;
      matrix.fillScreen(0);
      matrix.setTextColor(0);
      matrix.setCursor(cursor, i + 7);
      matrix.print("#");  //blank character
      matrix.setTextColor(CurrentColor);

      // Print tens digit of hour
      if (Clock_Hour < 10) {
        matrix.setCursor(cursor, i - 1);
        matrix.setTextColor(0);
        matrix.print("#");  //blank character
        matrix.setTextColor(CurrentColor);
      } else {
        matrix.setCursor(cursor, i - 1);
        matrix.print(String(Clock_Hour / 10));
      }
      cursor = cursor + font_width;  // Move to the next digit position

      // Print ones digit of hour
      matrix.setCursor(cursor, i + 7);
      matrix.setTextColor(0);
      matrix.print("#");
      matrix.setTextColor(CurrentColor);
      matrix.setCursor(cursor, i - 1);
      matrix.print(String(Clock_Hour % 10));
      cursor = cursor + font_width;

      // Print space between hour and minute
      cursor = cursor + 2;

      // Print tens digit of minute
      matrix.setCursor(cursor, i + 7);
      matrix.setTextColor(0);
      matrix.print("#");
      matrix.setTextColor(CurrentColor);
      matrix.setCursor(cursor, i - 1);
      matrix.print(String(Clock_Min / 10));
      cursor = cursor + font_width;

      // Print ones digit of minute
      matrix.setCursor(cursor, i + 7);
      matrix.setTextColor(0);
      matrix.print("#");
      matrix.setTextColor(CurrentColor);
      matrix.setCursor(cursor, i - 1);
      matrix.print(String(Clock_Min % 10));
      cursor = cursor + font_width;

      // Perform fade effect and update display
      // fade_tick(16, 2, 6);
      // matrix.show();
      for (int d = 0; d < slide_timing; d++) {
        fade_tick(16, i - 2, i - 6);  //to account for the slide?
        //matrix.show();
      }
    }
  } else if (Clock_Hour == Screen_Clock_Hour && Clock_Min == Screen_Clock_Min) {
    for (int i = 0; i < 9; i++) {
      cursor = start_cursor;
      matrix.fillScreen(0);

      // Print tens digit of hour
      if (Clock_Hour < 10) {
        matrix.setCursor(cursor, 7);
        matrix.setTextColor(0);
        matrix.print("#");  //blank character
        matrix.setTextColor(CurrentColor);
      } else {
        matrix.setCursor(cursor, 7);
        matrix.print(String(Clock_Hour / 10));
      }

      cursor = cursor + font_width;  // Move to the next digit position

      // Print ones digit of hour
      matrix.setCursor(cursor, 7);
      matrix.print(String(Clock_Hour % 10));
      cursor = cursor + font_width;

      // Print space between hour and minute
      cursor = cursor + 2;

      // Print tens digit of minute
      matrix.setCursor(cursor, 7);
      matrix.print(String(Clock_Min / 10));
      cursor = cursor + font_width;

      // Print ones digit of minute
      matrix.setCursor(cursor, 7);
      matrix.print(String(Clock_Min % 10));
      cursor = cursor + font_width;

      for (int d = 0; d < slide_timing; d++) {
        fade_tick(16, 2, 6);
        //matrix.show();
      }
    }
  } else if ((Clock_Hour / 10) != Screen_Clock_Hour / 10) {
    // Serial.println("refresh_normal_clock_slideEffect | (Clock_Hour / 10) != Screen_Clock_Hour / 10");
    // Loop for slide effect animation if tens digit of hour changes
    for (int i = 0; i < 9; i++) {
      cursor = start_cursor;
      matrix.fillScreen(0);

      // Print tens digit of hour
      matrix.setCursor(cursor, i + 7);
      matrix.print(String(Screen_Clock_Hour / 10));

      if (Clock_Hour < 10) {
        matrix.setCursor(cursor, i - 1);
        matrix.setTextColor(0);
        matrix.print("#");  //blank character
        matrix.setTextColor(CurrentColor);
      } else {
        matrix.setCursor(cursor, i - 1);
        matrix.print(String(Clock_Hour / 10));
      }

      cursor = cursor + font_width;  // Move to the next digit position

      // Print ones digit of hour
      matrix.setCursor(cursor, i + 7);
      matrix.print(String(Screen_Clock_Hour % 10));
      matrix.setCursor(cursor, i - 1);
      matrix.print(String(Clock_Hour % 10));
      cursor = cursor + font_width;

      // Print space between hour and minute
      cursor = cursor + 2;

      // Print tens digit of minute
      matrix.setCursor(cursor, i + 7);
      matrix.print(String(Screen_Clock_Min / 10));
      matrix.setCursor(cursor, i - 1);
      matrix.print(String(Clock_Min / 10));
      cursor = cursor + font_width;

      // Print ones digit of minute
      matrix.setCursor(cursor, i + 7);
      matrix.print(String(Screen_Clock_Min % 10));
      matrix.setCursor(cursor, i - 1);
      matrix.print(String(Clock_Min % 10));
      cursor = cursor + font_width;

      for (int d = 0; d < slide_timing; d++) {
        fade_tick(16, 2, 6);
        //matrix.show();
      }
    }
  } else if (Clock_Hour % 10 != Screen_Clock_Hour % 10) {
    // Serial.println("refresh_normal_clock_slideEffect | Clock_Hour % 10 != Screen_Clock_Hour % 10) ");
    // Loop for slide effect animation if ones digit of hour changes
    for (int i = 0; i < 9; i++) {
      cursor = start_cursor;
      matrix.fillScreen(0);

      // Print tens digit of hour
      if (Clock_Hour < 10) {
        matrix.setCursor(cursor, 7);
        matrix.setTextColor(0);
        matrix.print("#");  //blank character
        matrix.setTextColor(CurrentColor);
      } else {
        matrix.setCursor(cursor, 7);
        matrix.print(String(Clock_Hour / 10));
      }

      cursor = cursor + font_width;  // Move to the next digit position

      // Print ones digit of hour
      matrix.setCursor(cursor, i + 7);
      matrix.print(String(Screen_Clock_Hour % 10));
      matrix.setCursor(cursor, i - 1);
      matrix.print(String(Clock_Hour % 10));
      cursor = cursor + font_width;

      // Print space between hour and minute
      cursor = cursor + 2;

      // Print tens digit of minute
      matrix.setCursor(cursor, i + 7);
      matrix.print(String(Screen_Clock_Min / 10));
      matrix.setCursor(cursor, i - 1);
      matrix.print(String(Clock_Min / 10));
      cursor = cursor + font_width;

      // Print ones digit of minute
      matrix.setCursor(cursor, i + 7);
      matrix.print(String(Screen_Clock_Min % 10));
      matrix.setCursor(cursor, i - 1);
      matrix.print(String(Clock_Min % 10));
      cursor = cursor + font_width;

      for (int d = 0; d < slide_timing; d++) {
        fade_tick(16, 2, 6);
        //matrix.show();
      }
    }
  } else if (Clock_Min / 10 != Screen_Clock_Min / 10) {
    // Serial.println("refresh_normal_clock_slideEffect |(Clock_Min / 10 != Screen_Clock_Min / 10)");
    // Loop for slide effect animation if tens digit of minute changes
    for (int i = 0; i < 9; i++) {
      cursor = start_cursor;
      matrix.fillScreen(0);

      // Print tens digit of hour
      if (Clock_Hour < 10) {
        matrix.setCursor(cursor, 7);
        matrix.setTextColor(0);
        matrix.print("#");  //blank character
        matrix.setTextColor(CurrentColor);
      } else {
        matrix.setCursor(cursor, 7);
        matrix.print(String(Clock_Hour / 10));
      }

      cursor = cursor + font_width;  // Move to the next digit position

      // Print ones digit of hour
      matrix.setCursor(cursor, 7);
      matrix.print(String(Clock_Hour % 10));
      cursor = cursor + font_width;

      // Print space between hour and minute
      cursor = cursor + 2;

      // Print tens digit of minute
      matrix.setCursor(cursor, i + 7);
      matrix.print(String(Screen_Clock_Min / 10));
      matrix.setCursor(cursor, i - 1);
      matrix.print(String(Clock_Min / 10));
      cursor = cursor + font_width;

      // Print ones digit of minute
      matrix.setCursor(cursor, i + 7);
      matrix.print(String(Screen_Clock_Min % 10));
      matrix.setCursor(cursor, i - 1);
      matrix.print(String(Clock_Min % 10));
      cursor = cursor + font_width;

      // Perform fade effect and update display
      // fade_tick(16, 2, 6);
      // matrix.show();
      for (int d = 0; d < slide_timing; d++) {
        fade_tick(16, 2, 6);
        //matrix.show();
      }
    }
  } else if (Clock_Min % 10 != Screen_Clock_Min % 10) {
    // Loop for slide effect animation if ones digit of minute changes
    for (int i = 0; i < 9; i++) {
      cursor = start_cursor;
      matrix.fillScreen(0);

      // Print tens digit of hour
      if (Clock_Hour < 10) {
        matrix.setCursor(cursor, 7);
        matrix.setTextColor(0);
        matrix.print("#");  //blank character
        matrix.setTextColor(CurrentColor);
      } else {
        matrix.setCursor(cursor, 7);
        matrix.print(String(Clock_Hour / 10));
      }

      cursor = cursor + font_width;  // Move to the next digit position

      // Print ones digit of hour
      matrix.setCursor(cursor, 7);
      matrix.print(String(Clock_Hour % 10));
      cursor = cursor + font_width;

      // Print space between hour and minute
      cursor = cursor + 2;

      // Print tens digit of minute
      matrix.setCursor(cursor, 7);
      matrix.print(String(Clock_Min / 10));
      cursor = cursor + font_width;

      // Print ones digit of minute
      matrix.setCursor(cursor, i + 7);
      matrix.print(String(Screen_Clock_Min % 10));
      matrix.setCursor(cursor, i - 1);
      matrix.print(String(Clock_Min % 10));
      cursor = cursor + font_width;

      // Perform fade effect and update display
      // fade_tick(16, 2, 6);
      // matrix.show();
      for (int d = 0; d < slide_timing; d++) {
        fade_tick(16, 2, 6);
        // matrix.show();
      }
    }
  }

  // Update screen clock hour and minute
  Screen_Clock_Hour = Clock_Hour;
  Screen_Clock_Min = Clock_Min;
}

void tick(short int x_loc, short int y_loc_led1, short int y_loc_led2) {
short int correction;
  if ((millis() - tmp_millis) > 500 && (millis() - tmp_millis) <= 1000) {
    r_tmp = r;
    g_tmp = g;
    b_tmp = b;
  } else if ((millis() - tmp_millis) < 500 && (millis() - tmp_millis) <= 1000) {
    r_tmp = 0;
    g_tmp = 0;
    b_tmp = 0;

  } else if (((millis() - tmp_millis) > 1000) && ((millis() - tmp_millis) < 1999)) {
    correction = 1000 - (millis() - tmp_millis);
    tmp_millis = millis() - correction;
  } else if ((millis() - tmp_millis) >= 2000) {
    tmp_millis = millis() + country.ms();  //correct it with ms from eztime
                                           // tmp_millis = millis();
  }

    uint16_t color = matrix.Color(int(r_tmp), int(g_tmp), int(b_tmp));
    matrix.drawPixel(x_loc, y_loc_led1, color);
    matrix.drawPixel(x_loc, y_loc_led2, color);
    matrix.show();
}

void fade_tick(short int x_loc, short int y_loc_led1, short int y_loc_led2) {

  short int correction;

  if ((millis() - tmp_millis) > 500 && (millis() - tmp_millis) <= 1000) {
    if (r_tmp != r) {
      r_dec = double(r) / 23;
      r_tmp = (r_tmp + r_dec <= r) ? (r_tmp + r_dec) : r;
    }
    if (g_tmp != g) {
      g_dec = double(g) / 25;
      g_tmp = (g_tmp + g_dec <= g) ? (g_tmp + g_dec) : g;
    }
    if (b_tmp != b) {
      b_dec = double(b) / 24;
      b_tmp = (b_tmp + b_dec <= b) ? (b_tmp + b_dec) : b;
    }
    //fade in tick until 500miliseconds (1/2 second)
  } else if ((millis() - tmp_millis) < 500 && (millis() - tmp_millis) <= 1000) {
    if (r_tmp != 0) {
      r_dec = double(r) / 25;
      r_tmp = (r_tmp - r_dec >= 0) ? (r_tmp - r_dec) : 0;
    }
    if (g_tmp != 0) {
      g_dec = double(g) / 23;
      g_tmp = (g_tmp - g_dec >= 0) ? (g_tmp - g_dec) : 0;
    }
    if (b_tmp != 0) {
      b_dec = double(b) / 24;
      b_tmp = (b_tmp - b_dec >= 0) ? (b_tmp - b_dec) : 0;
    }
    //fade out tick until 1000miliseconds (full second)
  } else if (((millis() - tmp_millis) > 1000) && ((millis() - tmp_millis) < 1999)) {
    //in this case tmp_millis might be misaligned with the seconds, so this tries to correct it.
    correction = 1000 - (millis() - tmp_millis);
    tmp_millis = millis() - correction;
  } else if ((millis() - tmp_millis) >= 2000) {
    tmp_millis = millis() + country.ms();  //correct it with ms from eztime
  }

  uint16_t color = matrix.Color(int(r_tmp), int(g_tmp), int(b_tmp));
  matrix.drawPixel(x_loc, y_loc_led1, color);
  matrix.drawPixel(x_loc, y_loc_led2, color);
  matrix.show();
}

void randomNextColor(uint8_t maxRGBValue, int minutes_to_target) {
  // Generate a random component value equal to maxRGBValue
  uint8_t randomComponentIndex = random(3);
  next_r = (randomComponentIndex == 0) ? maxRGBValue : random(maxRGBValue);
  next_g = (randomComponentIndex == 1) ? maxRGBValue : random(maxRGBValue);
  next_b = (randomComponentIndex == 2) ? maxRGBValue : random(maxRGBValue);

  dec_next_r = (float)abs(r - next_r) / minutes_to_target;
  dec_next_g = (float)abs(g - next_g) / minutes_to_target;
  dec_next_b = (float)abs(b - next_b) / minutes_to_target;
}

void minimalist_Clock() {

  matrix.setBrightness(maxbrightness);
  //matrix.setTextColor(matrix.Color(r, g, b));
  //bool temperatureDisplayed = false
  short int tickXLoc;
  short int cursor = 0;

  if (country.hour() < 10) {
    tickXLoc = 4;
  } else {
    tickXLoc = 8;
  }

  if (Clock_Hour == country.hour() && Clock_Min == country.minute()) {  //clock_hour and clock_min didnt change
    minimalistic_Tick(tickXLoc, 4, 6);

    if (Clock_Min % 15 == 5) {  //every 15 minutes
      if (Clock_Hour < 10) {    //cursor changes depending on the digits of the clock since before 10h it is offset by 4
        cursor = 17;            //the c gets cut off but at least it fits
      } else {
        cursor = 20;
      }
      if (last_tempreading != Clock_Min) {
        // this part is only supposed to run once every 15 minutes
        sensors.requestTemperatures();  //request temperatures twice because the sensor seems to retain info from the last reading in the first request
        sensors.requestTemperatures();
        TempDataTimeHour.push(country.hour());             //push hour to an array that stores the 100 time and temperatures
        TempDataTimeMin.push(country.minute());            //push minutes
        TempDataSensor1.push(sensors.getTempCByIndex(0));  //push temperature

        last_tempreading = Clock_Min;                                     //update the last reading time
        matrix.setCursor(cursor, 4);                                      //set the cursor for the matrix print.
        matrix.print(String(int(round(TempDataSensor1.last()))) + "ºC");  //print on the screen the temperature
        ShowTemp = millis();                                              //the start time of the temperature display
        if (ShowTemp == 0) {
          ShowTemp = 1;  //in the unlikely event (1milisecond is not gonna matter)
        }
      }
      if (millis() - ShowTemp > 5000 && ShowTemp != 0) {  //after 5 seconds it should clear the matrix temperature data.
        matrix.setCursor(cursor, 4);
        matrix.setTextColor(0);
        matrix.print("####");
        matrix.setTextColor(CurrentColor);
        //Serial.println("CLEARING SCREEN");
        ShowTemp = 0;
      }
    }

  } else {  //clock changed

    if (Clock_Min % 35 == 0) {
      syncSecondsWithRTC();  //sync seconds every hour at the 35 min
    }

    Clock_Hour = country.hour();
    Clock_Min = country.minute();
    //if the time is between the sunset window.
    if ((((Clock_Hour * 100) + Clock_Min) >= ((sunrise_h * 100) + sunrise_m)) && ((Clock_Hour * 100 + Clock_Min) < (((sunrise_h + 1) * 100) + sunrise_m))) {

      if (sunrise_loop == 0) {  //first run
        // Convert colors
        RGB565TORGB(MainColor, next_r, next_g, next_b);
        RGB565TORGB(NightColor, r, g, b);

        //calculate minutes to reach target
        int dec_target_time = ((sunrise_h + 1) * 100 - (Clock_Hour * 100 + Clock_Min));
        // Calculate step sizes for each color component
        dec_next_r = (float)abs(r - next_r) / dec_target_time;
        dec_next_g = (float)abs(g - next_g) / dec_target_time;
        dec_next_b = (float)abs(b - next_b) / dec_target_time;

        // Initialize calculation variables
        dec_next_r_calc = r;
        dec_next_g_calc = g;
        dec_next_b_calc = b;

        sunrise_loop = 1;
      }

      // Gradually move each color component towards target
      if (dec_next_r_calc < next_r) dec_next_r_calc += dec_next_r;
      else if (dec_next_r_calc > next_r) dec_next_r_calc -= dec_next_r;

      if (dec_next_g_calc < next_g) dec_next_g_calc += dec_next_g;
      else if (dec_next_g_calc > next_g) dec_next_g_calc -= dec_next_g;

      if (dec_next_b_calc < next_b) dec_next_b_calc += dec_next_b;
      else if (dec_next_b_calc > next_b) dec_next_b_calc -= dec_next_b;

      // Convert to integers and set color
      r = int(dec_next_r_calc);
      g = int(dec_next_g_calc);
      b = int(dec_next_b_calc);

      CurrentColor = matrix.Color(r, g, b);
      TerminalOutput("r:" + String(r) + "g:" + String(g) + "b:" + String(b) + " | Target: " + String(next_r) + "," + String(next_g) + "," + String(next_b));
      matrix.setTextColor(CurrentColor);
    } else if ((((Clock_Hour * 100) + Clock_Min) > (((sunrise_h + 1) * 100) + sunrise_m)) && sunrise_loop == 0) {
      sunrise_loop = 1;
      RGB565TORGB(MainColor, r, g, b);
      CurrentColor = matrix.Color(r, g, b);
      matrix.setTextColor(CurrentColor);
    }

    matrix.fillScreen(0);
    //draw clock
    if (Clock_Hour < 10) {
      cursor = 0;
      matrix.setCursor(cursor, 7);
      matrix.print(String(Clock_Hour % 10));
      cursor = cursor + 2;
    } else {
      cursor = 0;
      matrix.setCursor(cursor, 7);
      matrix.print(String(Clock_Hour / 10));

      cursor = cursor + 3;
      cursor++;
      matrix.setCursor(cursor, 7);
      matrix.print(String(Clock_Hour % 10));
      cursor = cursor + 2;
    }
    cursor = cursor + 3;
    cursor++;
    matrix.setCursor(cursor, 7);
    matrix.print(String(Clock_Min / 10));

    cursor = cursor + 3;
    cursor++;
    matrix.setCursor(cursor, 7);
    matrix.print(String(Clock_Min % 10));

    matrix.show();
  }
}

void minimalistic_Tick(short int x_loc, short int y_loc_led1, short int y_loc_led2) {
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
        // r_dec = double(r) / 25;
        // g_dec = double(g) / 25;
        // b_dec = double(b) / 25;
        // matrix.setTextColor(matrix.Color(r, g, b));
        // TerminalOutput("Button Pressed!");

        getWeather();
        fadeOut(100);
        matrix.fillScreen(0);
        matrix.setBrightness(maxbrightness);
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
      TerminalOutput("rtc hour different than internal hour");
      DateTime rtcTime = rtc.now();
      rtc.adjust(DateTime(country.year(), country.month(), country.day(), country.hour(), country.minute(), rtcTime.second()));
      TerminalOutput("rtc hour adjusted");
      rtcTime = rtc.now();
    }

    unsigned long unixTime = rtcTime.unixtime();  // Get Unix timestamp from RTC
    country.setTime(unixTime);                    // Sync ezTime to RTC time
    LastRTCSyncTime = country.hour() * 100 + country.minute();
    TerminalOutput("time synced with rtc");
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
  (targetBrightness > maxbrightness) ? targetBrightness : maxbrightness;
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

void Server_SaveColor() {
  if (server.hasArg("Base_color") && server.hasArg("Night_color")) {
    String Base_color = server.arg("Base_color");
    String Night_color = server.arg("Night_color");

    int server_r = (int)strtol(&Base_color[1], NULL, 16) >> 16 & 0xFF;
    int server_g = (int)strtol(&Base_color[1], NULL, 16) >> 8 & 0xFF;
    int server_b = (int)strtol(&Base_color[1], NULL, 16) & 0xFF;

    r = server_r;
    g = server_g;
    b = server_b;
    maxRGBValue = max(max(r, g), b);  // Use max function directly for max RGB
    if (randomColor) {
      next_r = next_b = next_g = 0;
    }

    dec_next_r_calc = 0;  //this is used for the random color assignment
    dec_next_g_calc = 0;
    dec_next_b_calc = 0;
    r_dec = double(r) / 25;
    g_dec = double(g) / 25;
    b_dec = double(b) / 25;
    MainColor = matrix.Color(r, g, b);

    server_r = (int)strtol(&Night_color[1], NULL, 16) >> 16 & 0xFF;
    server_g = (int)strtol(&Night_color[1], NULL, 16) >> 8 & 0xFF;
    server_b = (int)strtol(&Night_color[1], NULL, 16) & 0xFF;

    NightColor = matrix.Color(server_r, server_g, server_b);
    EEPROM.put(0, MainColor);
    EEPROM.put(2, NightColor);
    EEPROM.commit();
    //matrix.setBrightness(server_brightness);
    //setBrightness(server_brightness);
    fadeOut(200);

    server.send(200, "text/plain", "Color set successfully");
    loop();

  } else {
    server.send(400, "text/plain", "Missing parameters");
  }
}


void TestColor() {
  if (server.hasArg("type") && server.hasArg("color")) {
    if (server.arg("type") == "base") {
      Screen_Clock_Hour = 88;
      Screen_Clock_Min = 88;
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
        out_text = "88:88";
        matrix.setCursor(4, 7);
        matrix.print(out_text);
        matrix.show();
      }
      loop();
    } else if (server.arg("type") == "night") {
      Screen_Clock_Hour = 88;
      Screen_Clock_Min = 88;
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
        out_text = "88:88";
        matrix.setCursor(0, 7);
        matrix.print(out_text);
        matrix.show();
      }
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

void Start_Web_Servers() {
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
    if (TempDataTimeHour.size() == 0) {
      TempDataTimeHour.push(country.hour());
      TempDataTimeMin.push(country.minute());
      TempDataSensor1.push(sensors.getTempCByIndex(0));
    }
    DateTime now = rtc.now();

    String formattedTimeString, formattedTempDataSensor1, formattedTerminalOutput;

    for (size_t i = 0; i < TempDataTimeHour.size(); ++i) {
      // Concatenate hour and minute with a colon
      formattedTimeString += "\"" + (TempDataTimeHour[i] < 10 ? "0" + String(TempDataTimeHour[i]) : String(TempDataTimeHour[i])) + ":" + (TempDataTimeMin[i] < 10 ? "0" + String(TempDataTimeMin[i]) : String(TempDataTimeMin[i])) + "\"";
      formattedTempDataSensor1 += String(TempDataSensor1[i]);
      // Add comma if not the last element
      if (i != TempDataTimeHour.size() - 1) {
        formattedTimeString += ",";
        formattedTempDataSensor1 += ",";
      }
    }
    for (size_t i = 0; i < TerminalBuffer.size(); ++i) {

      formattedTerminalOutput += TerminalBuffer[i] + "\n";
    }
    String page = String(MAIN_page);
    page.replace("{{WiFi}}", WiFi.SSID());
    page.replace("{{IP}}", WiFi.localIP().toString());
    page.replace("{{MAXBRIGHTNESS}}", String(maxbrightness));
    page.replace("{{POSIX}}", String(C_POSIX));
    page.replace("{{NIGHTCLOCKSTART}}", String(NightModeStartHour < 10 ? "0" + String(NightModeStartHour) : String(NightModeStartHour)) + ":" + String(NightModeStartMin < 10 ? "0" + String(NightModeStartMin) : String(NightModeStartMin)));
    page.replace("{{NIGHTCLOCKEND}}", String(NightModeEndHour < 10 ? "0" + String(NightModeEndHour) : String(NightModeEndHour)) + ":" + String(NightModeEndMin < 10 ? "0" + String(NightModeEndMin) : String(NightModeEndMin)));
    page.replace("{{MAINCOLOR}}", "#" + String(RGB565toRGB888(MainColor), HEX));
    page.replace("{{NIGHTCOLOR}}", "#" + String(RGB565toRGB888(NightColor), HEX));
    page.replace("{{FIRMWAREVERSION}}", "V:" + String(Version));
    page.replace("{{CURRENTHOUR}}", String(Clock_Hour) + ":" + String(Clock_Min));
    page.replace("{{ezTime}}", country.dateTime());
    page.replace("{{UPTIME}}", String(days) + "d " + String(hours) + "h " + String(minutes) + "m " + String(seconds) + "s");
    page.replace("{{RTCCHIP}}", String(now.day()) + "/" + String(now.month()) + "/" + String(now.year()) + "|" + String(now.hour()) + ":" + String(now.minute()) + ":" + String(now.second()));
    page.replace("{{SENSOR1VALUE}}", String(sensors.getTempCByIndex(0)) + "ºC");
    page.replace("{{SENSOR2VALUE}}", String(rtc.getTemperature()) + "ºC");
    page.replace("{{X_AXIS}}", formattedTimeString);
    page.replace("{{Y_SENSOR1DATA}}", formattedTempDataSensor1);
    page.replace("{{terminalOutput}}", formattedTerminalOutput);
    page.replace("{{PLACE}}", CIDADE);
    if (randomColor) {
      page.replace("{{RANDOMCOLORCHECKBOX}}", "checked ");
    } else {
      page.replace("{{RANDOMCOLORCHECKBOX}}", " ");
    }
    if (weather) {
      page.replace("{{WEATHERCHECKBOX}}", "checked ");
    } else {
      page.replace("{{WEATHERCHECKBOX}}", " ");
    }
    server.send(200, "text/html", page);
  });

  server.on("/JsonSensorData", HTTP_GET, []() {
    if (TempDataTimeHour.size() == 0) {
      sensors.requestTemperatures();
      TempDataTimeHour.push(country.hour());
      TempDataTimeMin.push(country.minute());
      TempDataSensor1.push(sensors.getTempCByIndex(0));
    }
    server.send(200, "application/json", "{\"Temperature\": " + String(TempDataSensor1.last()) + "}");
  });

  server.on("/Sensor", HTTP_GET, []() {
    if (TempDataTimeHour.size() == 0) {
      DateTime now = rtc.now();
      sensors.requestTemperatures();
      TempDataTimeHour.push(country.hour());
      TempDataTimeMin.push(country.minute());
      TempDataSensor1.push(sensors.getTempCByIndex(0));
    }


    String formattedTimeString, formattedTempDataSensor1, formattedTempDataSensor2;


    for (size_t i = 0; i < TempDataTimeHour.size(); ++i) {
      // Concatenate hour and minute with a colon
      formattedTimeString += "\"" + (TempDataTimeHour[i] < 10 ? "0" + String(TempDataTimeHour[i]) : String(TempDataTimeHour[i])) + ":" + (TempDataTimeMin[i] < 10 ? "0" + String(TempDataTimeMin[i]) : String(TempDataTimeMin[i])) + "\"";
      formattedTempDataSensor1 += String(TempDataSensor1[i]);
      // Add comma if not the last element
      if (i != TempDataTimeHour.size() - 1) {
        formattedTimeString += ",";
        formattedTempDataSensor1 += ",";
      }
    }

    String page = String(TempPage);
    page.replace("{{Custom-text}}", "V:" + String(Version) + "<br>João Fernandes");
    page.replace("{{X_AXIS}}", formattedTimeString);
    page.replace("{{Y_SENSOR1DATA}}", formattedTempDataSensor1);
    server.send(200, "text/html", page);
  });

  server.on("/Color", HTTP_GET, []() {
    String page = String(ColorPickerPage);
    float pageMaxBrightness = 10.0 - ((float)maxbrightness * 10.0 / 255.0);
    pageMaxBrightness = int(ceil(pageMaxBrightness));
    page.replace("{{Custom-text}}", "V:" + String(Version) + "<br>João Fernandes");
    page.replace("{{maxbrightness}}", String(pageMaxBrightness));
    page.replace("{{MAINCOLOR}}", "#" + String(RGB565toRGB888(MainColor), HEX));
    page.replace("{{NIGHTCOLOR}}", "#" + String(RGB565toRGB888(NightColor), HEX));
    server.send(200, "text/html", page);
  });

  server.on("/ESPReset", HTTP_POST, []() {
    ESP.restart();
  });

  server.on("/iro.min.js", HTTP_GET, []() {
    server.send_P(200, "text/javascript", iro_js);
  });

  server.on("/SaveColor", HTTP_POST, Server_SaveColor);
  server.on("/TestColor", HTTP_POST, TestColor);

  server.on("/TempRequest", HTTP_POST, []() {
    sensors.requestTemperatures();
    TempDataTimeHour.push(country.hour());
    TempDataTimeMin.push(country.minute());
    TempDataSensor1.push(sensors.getTempCByIndex(0));

    server.send(200, "text/plain", "temperatures requested sucessfully");
  });



  server.begin();
}
//if (last_weather_update_hour > 24 || last_weather_update_hour != Clock_Hour) {
void getWeather() {
  bool weather = true;

  HTTPClient http;  // Object of class HTTPClient
  http.useHTTP10(true);

  String url = "http://api.weatherapi.com/v1/forecast.json?key=" + APIKEY + "&q=" + CIDADE + "&days=1&aqi=no&alerts=no&lang=" + Language;
  if (Clock_Hour == 23) {
    url += "&hour=23";
  } else {
    url += "&hour=" + String(Clock_Hour + 1);
  }
  if (last_weather_update_hour > 24 || last_weather_update_hour != Clock_Hour) {  //only update if the last update was more than 1 hour ago or it was never run
    http.begin(wifiClient, url);                                                  // Begin HTTP request
    int httpCode = http.GET();                                                    // Get the response code

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
        TerminalOutput("deserializeJson() failed: ");
        TerminalOutput(error.f_str());
        return;
      }

      // Access the values in the filtered document
      JsonObject forecast_forecastday = doc["forecast"]["forecastday"][0]["hour"][0];
      const char* forecast_time = forecast_forecastday["time"];                    // e.g. "2021-09-24 23:00"
      float forecast_temp = forecast_forecastday["temp_c"];                        // e.g. 16.1
      int forecast_hum = forecast_forecastday["humidity"];                         // e.g. 80
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
      TerminalOutput(forecast_time);
      TerminalOutput(weather_str);
      TerminalOutput("Sunrise:" + String(sunrise_h) + ":" + String(sunrise_m));
    }

    http.end();  // Close the connection
  }
}


void TerminalOutput(String text) {
  text = String(Clock_Hour < 10 ? "0" : "") + Clock_Hour + ":" + (Clock_Min < 10 ? "0" : "") + Clock_Min + " : " + text;
  Serial.println(text);
  TerminalBuffer.push(text);
}
