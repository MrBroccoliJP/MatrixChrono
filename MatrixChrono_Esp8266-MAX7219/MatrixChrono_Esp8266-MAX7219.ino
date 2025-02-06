//JOAO FERNANDES 2021/2025

#include <ArduinoOTA.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
WiFiClient wifiClient;
#include <ESP8266WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <WiFiManager.h>
#include <ESP8266mDNS.h>
#include <ArduinoJson.h>
#include <MD_Parola.h>
#include <MD_MAX72xx.h>
#include <SPI.h>
#include "Fonts_data.h"
#include <time.h>
#include <ezTime.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#include <ArduinoJson.h>
#define BTN 5
#define DHTPIN 14
#define DHTTYPE DHT11
#include <EEPROM.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <CircularBuffer.h>
#include <StreamUtils.h>
#include <DallasTemperature.h>
#include <OneWire.h>
#include "HTML.h"
#include "secrets.h"

Timezone Portugal;
DHT dht(DHTPIN, DHTTYPE);

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");
String weekDays[7] = { "Domingo", "Segunda-feira", "Terça-feira", "Quarta-feira", "Quinta-feira", "Sexta-feira", "Sabado" };
String months[12] = { "Janeiro", "Fevereiro", "Março", "Abril", "Maio", "Junho", "Julho", "Agosto", "Setembro", "Outubro", "Novembro", "Dezembro" };
String s_version = "v1.37";

const int oneWireBus = 4;
OneWire oneWire(oneWireBus);
DallasTemperature sensors(&oneWire);

bool btnstate = 1;
bool prevstate = 1;
short int last_Update = 25;
unsigned int btntimer = 0;
int X = 1;
String s_hour;
String s_hour_dec;
String s_hour_uni;
String s_min;
String s_min_dec;
String s_min_uni;
String s_time_0;
String s_time_1;
String s_time_2;
String s_time_3;
String s_time_4;
char c_time_0[8];
char c_time_1[8];
char c_time_2[8];
char c_time_3[8];
char c_time_4[8];
char c_hour_uni[8];
char c_hour_dec[8];
char c_min_uni[8];
char c_min_dec[8];
char c_min[8];
char Date[64];
String tempHumStr;
char tempHum[64];
char tempHumtmp[64];
char outChar[64];
short int readSensor_last_update = 1;
short int real_h;
short int real_t;
short int real_hic;
String DateStr;
unsigned short int Days_since_last_sync = 50;  //default

//Variaveis para webserver
short int variable_len;
String CIDADE = "Aveiro";
char CIDADE_c[10];
bool weather = true;
bool ButtonEnable = true;
bool SensorEnable = true;
bool CheckDST = true;
short int ClockHourOverride = 0;
//short int array_len;
String POSIX = "CET-1CEST,M3.5.0/1,M10.5.0/2";  //default
String Language = "pt";
//short int POSIX_len;
uint8_t POSIX_len;
char tmp_c[64];
char POSIX_c[32];
char APIKEY_c[32];
char Language_c[6];
short unsigned int intensity = 0;
CircularBuffer<String, 100> SensorReadingsArr;
unsigned short int Start_m_clock = 2;  //DEFAULT
unsigned short int Stop_m_clock = 8;   //DEFAULT
bool Minimal_enable = true;            //DEFAULT
bool NightmodeScreenOff = false;       //DEFAULT TRUE
unsigned short int Syncday = 10;       //default 10
bool screenstate = true;
unsigned short int Font = 0;  //default

char lastsyncdate[16];

int clockHour;
int clockMin;
int clockSec;
uint8_t t_day;
uint8_t t_month;
uint16_t t_year;
bool SummerTime = false;
const char* forecast_time;
float forecast_temp = 0.0;
int forecast_hum = 0;
const char* forecast_condition = "null";
String weather_str;
char weather_c[64];

#define HARDWARE_TYPE MD_MAX72XX::FC16_HW

#define CLK_PIN 12   // or SCK
#define DATA_PIN 15  // or MOSI
#define CS_PIN 13    // or SS

#define MAX_DEVICES 4
#define NUM_ZONES 4

MD_Parola P = MD_Parola(HARDWARE_TYPE, DATA_PIN, CLK_PIN, CS_PIN, MAX_DEVICES);
textPosition_t d_hour_pos = PA_CENTER;  //default
textPosition_t i_hour_pos = PA_CENTER;
textPosition_t d_min_pos = PA_CENTER;
textPosition_t i_min_pos = PA_CENTER;

bool invert = true;  //WEB VARIABLE

unsigned long time_to_reach;
unsigned long m_time = 0;
unsigned long m = 0;
unsigned tdelay;

ESP8266WebServer server(80);
#include "Functions.h"

WiFiManager wm;


void setup() {
  Serial.begin(9600);

  pinMode(BTN, INPUT_PULLUP);

  P.begin(5);
  P.setZone(0, 0, 3);
  P.setZoneEffect(0, invert, PA_FLIP_LR);
  P.setZoneEffect(0, invert, PA_FLIP_UD);
  P.setZone(4, 0, 0);
  P.setZoneEffect(1, invert, PA_FLIP_LR);
  P.setZoneEffect(1, invert, PA_FLIP_UD);
  P.setZone(3, 1, 1);
  P.setZoneEffect(2, invert, PA_FLIP_LR);
  P.setZoneEffect(2, invert, PA_FLIP_UD);
  P.setZone(2, 2, 2);
  P.setZoneEffect(3, invert, PA_FLIP_LR);
  P.setZoneEffect(3, invert, PA_FLIP_UD);
  P.setZone(1, 3, 3);
  P.setZoneEffect(4, invert, PA_FLIP_LR);
  P.setZoneEffect(4, invert, PA_FLIP_UD);

  P.setFont(ExtASCII);
  P.setIntensity(5);

  P.displayZoneText(0, s_version.c_str(), PA_LEFT, 75, 1000, PA_MESH, PA_NO_EFFECT);
  while (!P.getZoneStatus(0)) {
    P.displayAnimate();
  }
  //reset settings - wipe   redentials for testing
  wm.setAPCallback(configModeCallback);
  WiFi.mode(WIFI_STA);  // explicitly set mode, esp defaults to STA+AP
  WiFi.setSleep(WIFI_PS_NONE);
  //wm.setConfigPortalBlocking(false);  // Set configuration portal to non-blocking mode
  wm.setConfigPortalTimeout(120);  // Set configuration portal timeout to 60 seconds
                                   // wifiportal = wm.autoConnect("CLOCKAP");

 if (!wm.autoConnect("ClockAP")) {
 //if (!wm.autoConnect()) {
    Serial.println("failed to connect and hit timeout");
    //reset and try again, or maybe put it to deep sleep
    ESP.restart();
    delay(1000);
  }

  Serial.println("connected...yeey :)");
  P.displayZoneText(0, "  Wifi Connected  ", PA_LEFT, 50, 1000, PA_SCROLL_RIGHT, PA_NO_EFFECT);
  while (!P.getZoneStatus(0)) {

    P.displayAnimate();
    yield();
  }

  char IP[20];
  sprintf(IP, "%03d:%03d:%03d:%03d", WiFi.localIP()[0], WiFi.localIP()[1], WiFi.localIP()[2], WiFi.localIP()[3]);

  P.displayZoneText(0, IP, PA_LEFT, 50, 1000, PA_SCROLL_RIGHT, PA_NO_EFFECT);
  while (!P.getZoneStatus(0)) {

    P.displayAnimate();
    yield();
  }

  ArduinoOTA.setPassword("admin");
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else {  // U_FS
      type = "filesystem";
    }

    // NOTE: if updating FS this would be the place to unmount FS using FS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  server.on("/", handleRoot);             //Which routine to handle at root location
  server.on("/action_page", handleForm);  //form action is handled here
  server.on("/Sensor", SensorReadingsPage);
  server.on("/JsonSensorData", JsonSensorData);
  server.on("/DELETEALL", deleteall);
  server.on("/turnOnOffscreen", turnOnOffscreen);
  server.on("/Reboot", reboot);
  server.begin();  //Start server
  Serial.println("HTTP server started");

  EEPROM.begin(200);

  //Serial.println("here");
  int tmp = 0;
  variable_len = EEPROM.read(tmp);  //cdade lengh
  while (variable_len > 50 || variable_len == 0) {
    server.handleClient();
  }
  Serial.print("\nCIDADE_lENGHT:");
  Serial.println(variable_len);
  tmp++;
  Serial.print("\nTMP: ");
  Serial.println(tmp);
  for (short int i = tmp; i < (variable_len + tmp); i++) {
    CIDADE_c[i - tmp] = EEPROM.read(i);
  }
  CIDADE = String(CIDADE_c);
  Serial.print("\nCIDADE:");
  Serial.println(CIDADE);
  tmp = tmp + variable_len;
  Serial.print("\nTMP: ");
  Serial.println(tmp);
  variable_len = EEPROM.read(tmp);
  while (variable_len > 50 || variable_len == 0) {
    server.handleClient();
  }
  tmp++;
  Serial.print("\nTMP: ");
  Serial.println(tmp);
  Serial.print("\nAPI_lENGHT:");
  Serial.println(variable_len);

  for (short int i = tmp; i < (variable_len + tmp); i++) {
    APIKEY_c[i - tmp] = EEPROM.read(i);
  }

  APIKEY = String(APIKEY_c);
  Serial.print("\nAPIKEY:");
  Serial.println(APIKEY);

  tmp = tmp + variable_len;
  Serial.print("\nTMP: ");
  Serial.println(tmp);

  variable_len = EEPROM.read(tmp);
  //variable_len =152;
  while (variable_len > 50 || variable_len == 0) {
    server.handleClient();
  }
  tmp++;
  Serial.print("\nTMP: ");
  Serial.println(tmp);
  Serial.print("\nPOSIX_lENGHT:");
  Serial.println(variable_len);

  for (short int i = tmp; i < (variable_len + tmp); i++) {
    POSIX_c[i - tmp] = EEPROM.read(i);
    //Serial.println(POSIX_c[i - tmp]);
  }

  POSIX = String(POSIX_c);
  Serial.print("\nPOSIX:");
  Serial.println(POSIX);
  Portugal.setPosix(POSIX);
  tmp = tmp + variable_len;
  Serial.print("\nTMP: ");
  Serial.println(tmp);

  //LANGUAGE SETUP
  variable_len = EEPROM.read(tmp);
  Serial.print("\nTMP: ");
  Serial.println(tmp);
  Serial.print("\nLanguage_lENGHT:");
  Serial.println(variable_len);
  while (variable_len != 2) {
    server.handleClient();
  }
  tmp++;
  for (short int i = tmp; i < (variable_len + tmp); i++) {
    Language_c[i - tmp] = EEPROM.read(i);
    //Serial.println(Language_c[i - tmp]);
  }
  Language = String(Language_c);
  Serial.print("\nLanguage:");
  Serial.println(Language);
  tmp = tmp + variable_len;

  SensorEnable = EEPROM.read(tmp);
  Serial.print("\nSENSOR ENABLE:");
  Serial.println(SensorEnable);

  tmp++;
  Serial.print("\nTMP: ");
  Serial.println(tmp);

  CheckDST = EEPROM.read(tmp);
  Serial.print("\nDST ENABLE:");
  Serial.println(CheckDST);
  tmp++;
  Serial.print("\nTMP: ");
  Serial.println(tmp);

  ButtonEnable = EEPROM.read(tmp);
  Serial.print("\nBUTTON ENABLE:");
  Serial.println(ButtonEnable);
  tmp++;
  Serial.print("\nTMP: ");
  Serial.println(tmp);

  ClockHourOverride = EEPROM.read(tmp);
  if (ClockHourOverride > 12) {
    ClockHourOverride = 12 - ClockHourOverride;
  }
  Serial.print("\nCLOCK OVERRIDE:");
  Serial.println(ClockHourOverride);
  tmp++;
  Serial.print("\nTMP: ");
  Serial.println(tmp);
  intensity = EEPROM.read(tmp);
  Serial.print("\nIntensity: ");
  Serial.println(intensity);

  tmp++;
  Minimal_enable = EEPROM.read(tmp);
  Serial.print("Night clock Enable: ");
  Serial.println(Minimal_enable);
  tmp++;

  //  if ( Minimal_enable == true) {
  if (EEPROM.read(tmp) == EEPROM.read(tmp + 1) || EEPROM.read(tmp) > 23 || EEPROM.read(tmp + 1) > 23) {
    EEPROM.write(tmp, Start_m_clock);
    tmp++;
    EEPROM.write(tmp, Stop_m_clock);
    EEPROM.commit();
    tmp++;
    Serial.println("SETTING DEFAULT VARIABLE FOR NIGHT CLOCK");
  }

  else {
    Start_m_clock = EEPROM.read(tmp);
    Serial.print("START TIME: ");
    Serial.println(Start_m_clock);
    tmp++;

    Stop_m_clock = EEPROM.read(tmp);
    Serial.print("STOP TIME: ");
    Serial.println(Stop_m_clock);
    tmp++;
  }

  if (EEPROM.read(tmp) != 0 && EEPROM.read(tmp) != 1) {
    NightmodeScreenOff = false;  //default
    Serial.println("NIGHT MODE SCREEN OFF DISABLED -- DEFAULT");
  } else {
    NightmodeScreenOff = EEPROM.read(tmp);
    Serial.print("\nNightmodeScreenoff: ");
    Serial.println(NightmodeScreenOff);
  }

  tmp++;

  Syncday = EEPROM.read(tmp);
  Syncday = constrain(Syncday, 2, 30);
  Serial.print("\n SYNCDAY: ");
  Serial.println(Syncday);
  Serial.print("\nSYNCDAY LOCATION: ");
  Serial.println(tmp);

  tmp++;

  Font = EEPROM.read(tmp);
  if (Font > 4) {
    Font = 1;  //DEFAULT
    Serial.println("DEFAULT FONT SET Err: >2");
  }
  Serial.print("\n Font: ");
  Serial.println(Font);
  Serial.print("\nFont LOCATION: ");
  Serial.println(tmp);

  tmp++;


  for (int i = 0; i < 200; i++) {
    char a = char(EEPROM.read(i));
    Serial.print(a);
  }


  dht.begin();
  timeClient.begin();

  P.setIntensity(intensity);

  getTime();

  convert(Date, outChar);
  P.displayZoneText(0, outChar, PA_LEFT, 50, 1500, PA_SCROLL_RIGHT, PA_NO_EFFECT);
  while (!P.getZoneStatus(0)) {

    P.displayAnimate();
    yield();
  }


  if (Font == 4) {
    d_hour_pos = PA_LEFT;
    i_hour_pos = PA_CENTER;
    d_min_pos = PA_CENTER;
    i_min_pos = PA_LEFT;
  } else {
    d_hour_pos = PA_LEFT;
    i_hour_pos = PA_CENTER;
    d_min_pos = PA_CENTER;
    i_min_pos = PA_CENTER;
  }
}
void loop() {
  Serial.print("\nSTart: ");
  Serial.println(Start_m_clock);
  Serial.print("\nSTop: ");
  Serial.println(Stop_m_clock);
  Serial.print("\nMinimal: ");
  Serial.print(Minimal_enable);

  if (Minimal_enable == true) {
    Serial.println("\n Minimalchecktrue");
    while (Minimal_enable = true) {


      if (Start_m_clock > Stop_m_clock) {
        Serial.println("\n start>stop");
        if ((clockHour >= Start_m_clock && clockHour <= 24) || (clockHour >= 0 && clockHour < Stop_m_clock)) {
          Serial.println("\n InsideMinimalclock");
          P.displayClear();
          P.setFont(minimal);
          if (NightmodeScreenOff == true) {
            P.displayShutdown(1);
          } else {
            P.displayShutdown(0);
          }
          while ((clockHour >= Start_m_clock && clockHour <= 24) || (clockHour >= 0 && clockHour < Stop_m_clock)) {
            server.handleClient();  //Handle client requests
            ArduinoOTA.handle();
            minimalist_clock();
          }
        } else {
          P.displayClear();

          if (Font == 2) {
            P.setFont(numeric7Seg);
          } else if (Font == 4) {
            P.setFont(regular);
          } else {
            P.setFont(ExtASCII);
          }
          update_all();

          while (clockHour >= Stop_m_clock && clockHour < Start_m_clock) {
            server.handleClient();  //Handle client requests
            ArduinoOTA.handle();
            normal_clock();
          }
        }

      }


      else if (Start_m_clock < Stop_m_clock) {
        if (clockHour >= Start_m_clock && clockHour < Stop_m_clock) {
          P.displayClear();
          P.setFont(minimal);
          if (NightmodeScreenOff == true) {
            P.displayShutdown(1);
          } else {
            P.displayShutdown(0);
          }
          while (clockHour >= Start_m_clock && clockHour < Stop_m_clock) {
            server.handleClient();  //Handle client requests
            ArduinoOTA.handle();
            minimalist_clock();
          }
        }

        else {
          P.displayClear();
          if (Font == 2) {
            P.setFont(numeric7Seg);
          } else if (Font == 3) {
            P.setFont(BOLD);
          } else if (Font == 4) {
            P.setFont(regular);
          } else {
            P.setFont(ExtASCII);
          }
          update_all();

          while (clockHour < Start_m_clock || clockHour >= Stop_m_clock) {
            server.handleClient();  //Handle client requests
            ArduinoOTA.handle();
            normal_clock();
          }
        }
      }
    }
  } else {
    P.displayClear();
    P.setFont(numeric7Seg);
    update_all();
    while (Minimal_enable == false) {
      server.handleClient();  //Handle client requests
      ArduinoOTA.handle();
      normal_clock();
    }
  }



}


void normal_clock() {
  timekeep();

  if (clockMin % 20 == 0 && clockSec % 2 == 0 && clockSec < 30 && readSensor_last_update != clockSec) {
    readSensor_last_update = clockSec;
    if (SensorEnable) {
      readSensor();
      Serial.println("READ SENSOR");
      Serial.println(tempHum);
      //readSensor_last_update = clockSec;
    }
  } else if (clockSec == 30 && clockMin % 20 == 0 && readSensor_last_update != clockSec) {
    readSensor_last_update = clockSec;
    if (SensorEnable) {
      showtemps(true, true);
    } else {
      showtemps(false, true);
    }
  } else if (Days_since_last_sync >= Syncday && clockHour == 5 && clockSec == 41) {
    getTime();
    Days_since_last_sync = 0;
  }

  if (ButtonEnable) {
    btnstate = digitalRead(BTN);
    if (btnstate == LOW && btntimer == 0) {
      prevstate = LOW;
      btntimer = millis();
      //showtemps();
    }
    //btnstate = digitalRead(BTN);
    if (prevstate == LOW && btnstate == HIGH) {
      btntimer = 0;
      prevstate = HIGH;
      showtemps(false, true);
      //getTime();
    } else if (btnstate == LOW && prevstate == LOW && millis() - btntimer >= 5000) {
      ESP.reset();
    }
  }
  //  else{
  //  btnstate = HIGH;
  //  }


  if (clockSec == 60) {
    //getTime();
    clockSec = 0;
    clockMin++;


    if (clockMin < 60 && clockMin % 10 == 0) {

      P.displayZoneText(1, c_min_uni, i_min_pos, 50, 0, PA_NO_EFFECT, PA_SCROLL_UP);
      P.displayZoneText(2, c_time_2, d_min_pos, 50, 0, PA_NO_EFFECT, PA_SCROLL_UP);
      P.displayAnimate();
      while (!P.getZoneStatus(1) && !P.getZoneStatus(2)) {

        P.displayAnimate();
      }
      updatetime_d();
    } else if (clockMin < 60 && clockMin % 10 != 0) {
      P.displayZoneText(1, c_min_uni, i_min_pos, 50, 0, PA_NO_EFFECT, PA_SCROLL_UP);
      while (!P.getZoneStatus(1)) {
        displayTime();
        P.displayAnimate();
      }
      updatetime_u();
    }

    else if (clockMin == 60) {
      clockMin = 0;
      clockHour++;

      if (clockHour == 24) {
        Days_since_last_sync++;
        clockMin = 0;
        clockHour = 0;

        if (t_month == 2 && t_day == 28) {
          if (checkLeapYear(t_year)) {
            t_day = 29;
          } else {
            t_day = 1;
            t_month++;
          }
        } else if (t_month == 2 && t_day == 29) {
          t_day = 1;
          t_month++;
        } else if (t_month == 12 && t_day == 31) {
          t_day = 1;
          t_month = 1;
          t_year++;
        }

        else if ((t_month == 1 || t_month == 3 || t_month == 5 || t_month == 7 || t_month == 8 || t_month == 10) && t_day == 31) {
          t_day = 1;
          t_month++;
        } else if ((t_month == 4 || t_month == 6 || t_month == 9 || t_month == 11) && t_day == 30) {
          t_day = 1;
          t_month++;
        } else {
          t_day++;
        }
        //clockSec = 0;
        //displayTime();
        P.displayZoneText(1, c_min_uni, i_min_pos, 50, 0, PA_NO_EFFECT, PA_SCROLL_UP);
        P.displayZoneText(2, c_time_2, d_min_pos, 50, 0, PA_NO_EFFECT, PA_SCROLL_UP);
        P.displayZoneText(3, c_hour_uni, i_hour_pos, 50, 0, PA_NO_EFFECT, PA_SCROLL_UP);
        P.displayZoneText(4, c_hour_dec, d_hour_pos, 50, 0, PA_NO_EFFECT, PA_SCROLL_UP);
        while (!P.getZoneStatus(1) && !P.getZoneStatus(2) && !P.getZoneStatus(3) && !P.getZoneStatus(4)) {

          P.displayAnimate();
        }
        update_all();
      } else if (clockHour == 1 && t_month == 3 && t_day > 20) {
        checkSummerTime();
        //Serial.println("CHECK_SUMMER_TIME = YES");

      } else if (clockHour == 2 && t_month == 10 && t_day > 20) {
        checkSummerTime();
        //Serial.println("CHECK_SUMMER_TIME = NO"
      }

      else if (clockHour % 10 == 0) {
        //clockHour++;
        Serial.println("clockHour % 10 == 0 ");
        P.displayAnimate();
        P.displayZoneText(1, c_min_uni, i_min_pos, 50, 0, PA_NO_EFFECT, PA_SCROLL_UP);
        P.displayZoneText(2, c_time_2, d_min_pos, 50, 0, PA_NO_EFFECT, PA_SCROLL_UP);
        P.displayZoneText(3, c_hour_uni, i_hour_pos, 50, 0, PA_NO_EFFECT, PA_SCROLL_UP);
        P.displayZoneText(4, c_hour_dec, d_hour_pos, 50, 0, PA_NO_EFFECT, PA_SCROLL_UP);
        while (!P.getZoneStatus(1) && !P.getZoneStatus(2) && !P.getZoneStatus(3) && !P.getZoneStatus(4)) {

          P.displayAnimate();
        }
        update_all();
      } else {
        //clockHour++;
        P.displayZoneText(1, c_min_uni, i_min_pos, 50, 0, PA_NO_EFFECT, PA_SCROLL_UP);
        P.displayZoneText(2, c_time_2, d_min_pos, 50, 0, PA_NO_EFFECT, PA_SCROLL_UP);
        P.displayZoneText(3, c_hour_uni, i_hour_pos, 50, 0, PA_NO_EFFECT, PA_SCROLL_UP);
        while (!P.getZoneStatus(1) && !P.getZoneStatus(2) && !P.getZoneStatus(3)) {

          P.displayAnimate();
        }
        updatehour_u();
      }
    }
  }
  //  else {
  //
  //  }
  displayTime();
}

void minimalist_clock() {
  timekeep();

  if (clockMin % 20 == 0 && clockSec % 2 == 0 && clockSec < 30 && readSensor_last_update != clockSec) {
    readSensor_last_update = clockSec;
    if (SensorEnable) {
      readSensor();
      Serial.println("READ SENSOR");
      Serial.println(tempHum);
      //readSensor_last_update = clockSec;
    }
  } else if (clockSec == 30 && clockMin % 20 == 0 && readSensor_last_update != clockSec) {
    readSensor_last_update = clockSec;
    if (SensorEnable) {
      readSensor();
      DateStr = "   " + String(t_day) + "/" + String(t_month) + " | ";
      DateStr.toCharArray(Date, 64);
      Serial.println(DateStr);
      if (clockMin == 0) {
        SensorReadingsArr.unshift(String(tempHum) + " " + String(DateStr) + " " + String(clockHour) + ":" + String(clockMin) + "0");
      } else {
        SensorReadingsArr.unshift(String(tempHum) + " " + String(DateStr) + " " + String(clockHour) + ":" + String(clockMin));
      }
    } else {
      SensorReadingsArr.unshift("SensorDisabled");
    }
  } else if (Days_since_last_sync >= Syncday && clockHour == 5 && clockSec == 41) {
    getTime();
    Days_since_last_sync = 0;
  }

  if (ButtonEnable) {
    btnstate = digitalRead(BTN);
    if (btnstate == LOW && btntimer == 0) {
      prevstate = LOW;
      btntimer = millis();
      //showtemps();
    }
    //btnstate = digitalRead(BTN);
    if (prevstate == LOW && btnstate == HIGH) {
      btntimer = 0;
      prevstate = HIGH;
      //showtemps(false, true);
    } else if (btnstate == LOW && prevstate == LOW && millis() - btntimer >= 5000) {
      ESP.reset();
    }
  }

  if (clockSec == 60) {
    //getTime();
    clockSec = 0;
    clockMin++;


    if (clockMin == 60) {
      clockMin = 0;
      clockHour++;

      if (clockHour == 24) {
        //getTime();
        clockMin = 0;
        clockHour = 0;
        Days_since_last_sync++;

        if (t_month == 2 && t_day == 28) {
          if (checkLeapYear(t_year)) {
            t_day = 29;
          } else {
            t_day = 1;
            t_month++;
          }
        } else if (t_month == 2 && t_day == 29) {
          t_day = 1;
          t_month++;
        } else if (t_month == 12 && t_day == 31) {
          t_day = 1;
          t_month = 1;
          t_year++;
        }

        else if ((t_month == 1 || t_month == 3 || t_month == 5 || t_month == 7 || t_month == 8 || t_month == 10) && t_day == 31) {
          t_day = 1;
          t_month++;
        } else if ((t_month == 4 || t_month == 6 || t_month == 9 || t_month == 11) && t_day == 30) {
          t_day = 1;
          t_month++;
        } else {
          t_day++;
        }

      } else if (clockHour == 1 && t_month == 3 && t_day > 20) {
        checkSummerTime();
        //Serial.println("CHECK_SUMMER_TIME = YES");

      } else if (clockHour == 2 && t_month == 10 && t_day > 20) {
        checkSummerTime();
        //Serial.println("CHECK_SUMMER_TIME = NO");
      }
    }
  }

  displayTime_minimalist();
}
