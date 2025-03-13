/*
 * MatrixChrono - IoT Smart Home Clock System
 * Copyright (c) 2025 João Fernandes
 * 
 * This work is licensed under the Creative Commons Attribution-NonCommercial 
 * 4.0 International License. To view a copy of this license, visit:
 * http://creativecommons.org/licenses/by-nc/4.0/
 */

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

void checkSummerTime() {
  if (CheckDST) {
    setTime(clockHour, clockMin, clockSec, t_day, t_month, t_year);
    if (SummerTime < Portugal.isDST()) {
      clockHour++;
      SummerTime = true;
      Serial.println("CHECK SUMMER TIME = YES ++");
      Serial.print("From ");
      Serial.print(clockHour - 1);
      Serial.print(" to ");
      Serial.println(clockHour);
      //
    } else if (SummerTime > Portugal.isDST()) {
      clockHour--;
      SummerTime = false;
      Serial.println("CHECK SUMMER TIME = NO --");
      Serial.print("From ");
      Serial.print(clockHour + 1);
      Serial.print(" to ");
      Serial.println(clockHour);
    } else {
      Serial.println("CHECK SUMMER TIME = NO CHANGE");
    }
  } else {
    SummerTime = false;
    Serial.println("CHECK SUMMER TIME = DISABLED");
  }
}

void getWeather() {
  weather = true;

  HTTPClient http;  //Object of class HTTPClient
  http.useHTTP10(true);

  if (clockHour == 23) {
    http.begin(wifiClient, "http://api.weatherapi.com/v1/forecast.json?key=" + APIKEY + "&q=" + CIDADE + "&days=1&aqi=no&alerts=no&lang=" + Language + "&hour=" + "23");
  } else {
    http.begin(wifiClient, "http://api.weatherapi.com/v1/forecast.json?key=" + APIKEY + "&q=" + CIDADE + "&days=1&aqi=no&alerts=no&lang=" + Language + "&hour=" + String(clockHour + 1));
  }

  int httpCode = http.GET();
  if (httpCode <= 0) {
    weather = false;
  } else {
    Stream& input = http.getStream();

    StaticJsonDocument<160> filter;

    JsonObject filter_forecast = filter["forecast"]["forecastday"][0]["hour"].createNestedObject();
    filter_forecast["temp_c"] = true;
    filter_forecast["time"] = true;
    filter_forecast["humidity"] = true;
    filter_forecast["condition"]["text"] = true;

    StaticJsonDocument<384> doc;
    ReadBufferingStream bufferedFile(input, 64);
    //deserializeJson(doc, bufferedFile);
    DeserializationError error = deserializeJson(doc, bufferedFile, DeserializationOption::Filter(filter));

    if (error) {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.f_str());
      return;
    }

    JsonObject forecast_forecastday = doc["forecast"]["forecastday"][0]["hour"][0];
    forecast_time = forecast_forecastday["time"];    // "2021-09-24 ...
    forecast_temp = forecast_forecastday["temp_c"];  // 16.1
    forecast_hum = forecast_forecastday["humidity"];
    short int tmp_t = round(forecast_temp);

    forecast_condition = forecast_forecastday["condition"]["text"];

    weather_str = String(forecast_condition) + " | " + forecast_hum + "%" + " | " + String(tmp_t) + "ºC";
    weather_str.toCharArray(weather_c, 64);
    Serial.println(forecast_time);
    //  Serial.println(forecast_temp);
    //  Serial.println(forecast_hum);
    //  Serial.println(forecast_condition);
    Serial.println(weather_str);
  }
  http.end();
}

void getTime() {
  short int i = 0;
  bool updated = timeClient.update();  // Store result of update
  Serial.println(updated);

  if (!updated) {
    P.displayZoneText(0, ". . . .", PA_CENTER, 0, 0, PA_PRINT, PA_NO_EFFECT);
    P.displayAnimate();

    int retryCount = 0;
    while (!timeClient.update() && retryCount < 10) {  // Add retry limit
      Serial.println("Failed to connect to NTP");

      // Simplified animation using modulo
      const char* patterns[] = { "  . . .", ".   . .", ". .   .", ". . .  ", ". .   .", ".   . ." };
      P.displayZoneText(0, patterns[i % 6], PA_CENTER, 0, 0, PA_PRINT, PA_NO_EFFECT);
      P.displayAnimate();

      i++;
      retryCount++;
      delay(500);  // Avoid spamming the NTP server
    }
    if (retryCount == 10) {
      Serial.println("NTP update failed after 10 retries.");
      delay(5000);
      getTime();
      //return;  // Exit function if NTP sync fails
    }
  }

  Serial.println("Time updated");

  // Fetch time details
  unsigned long epochTime = timeClient.getEpochTime();
  Serial.print("Epoch Time: ");
  Serial.println(epochTime);

  Serial.print("Formatted Time: ");
  Serial.println(timeClient.getFormattedTime());

  clockHour = timeClient.getHours() + ClockHourOverride;
  if (clockHour >= 24) clockHour -= 24;

  clockMin = timeClient.getMinutes();
  clockSec = timeClient.getSeconds();

  // Get weekday
  String weekDay = weekDays[timeClient.getDay()];

  // Convert epoch time to human-readable format
  time_t rawtime = epochTime;
  struct tm* ti = localtime(&rawtime);
  t_year = ti->tm_year + 1900;
  t_month = ti->tm_mon + 1;
  t_day = ti->tm_mday;

  Serial.println(t_year);
  Serial.println(t_month);
  Serial.println(t_day);

  // Format date string
  sprintf(lastsyncdate, "%i/%i/%i", t_day, t_month, t_year);
  DateStr = String("   ") + (t_day < 10 ? "0" : "") + String(t_day) + " de " + String(months[t_month - 1]) + " de " + String(t_year) + "  ";

  Serial.println(DateStr);
  DateStr.toCharArray(Date, 64);

  SummerTime = false;
  checkSummerTime();
  if (clockHour >= 24) clockHour -= 24;
}



bool checkLeapYear(int theYear) {
  if (theYear % 4 != 0) return false;
  if (theYear % 100 == 0 && theYear % 400 != 0) return false;
  if (theYear % 400 == 0) return true;
  return true;
}



void displayTime() {

  s_min_dec = String(clockMin / 10);

  if (Font == 2) {
    s_time_0 = " " + s_min_dec;
    s_time_1 = ":" + s_min_dec;
    s_time_2 = "¢" + s_min_dec;
    s_time_3 = "£" + s_min_dec;
    s_time_4 = "¤" + s_min_dec;
  } else {
    s_time_0 = " " + s_min_dec;
    s_time_1 = ":" + s_min_dec;
    s_time_2 = s_time_1;
    s_time_3 = s_time_1;
    s_time_4 = s_time_1;
  }
  s_time_0.toCharArray(c_time_0, 8);
  s_time_1.toCharArray(c_time_1, 8);
  s_time_2.toCharArray(c_time_2, 8);
  s_time_3.toCharArray(c_time_3, 8);
  s_time_4.toCharArray(c_time_4, 8);

  if (millis() - m < 500) {
    P.displayZoneText(2, c_time_1, d_min_pos, 0, 0, PA_PRINT, PA_NO_EFFECT);
    P.displayAnimate();
  } else if (millis() - m < 1000) {

    P.displayZoneText(2, c_time_0, d_min_pos, 0, 0, PA_PRINT, PA_NO_EFFECT);
    P.displayAnimate();

  } else if (millis() - m < 1500) {

    P.displayZoneText(2, c_time_2, d_min_pos, 0, 0, PA_PRINT, PA_NO_EFFECT);
    P.displayAnimate();

  } else if (millis() - m < 2000) {

    P.displayZoneText(2, c_time_0, d_min_pos, 0, 0, PA_PRINT, PA_NO_EFFECT);
    P.displayAnimate();

  } else if (millis() - m < 2500) {

    P.displayZoneText(2, c_time_3, d_min_pos, 0, 0, PA_PRINT, PA_NO_EFFECT);
    P.displayAnimate();

  } else if (millis() - m < 3000) {

    P.displayZoneText(2, c_time_0, d_min_pos, 0, 0, PA_PRINT, PA_NO_EFFECT);
    P.displayAnimate();

  } else if (millis() - m < 3500) {

    P.displayZoneText(2, c_time_4, d_min_pos, 0, 0, PA_PRINT, PA_NO_EFFECT);
    P.displayAnimate();

  } else if (millis() - m < 4000) {

    P.displayZoneText(2, c_time_0, d_min_pos, 0, 0, PA_PRINT, PA_NO_EFFECT);
    P.displayAnimate();

  } else if (millis() - m < 4500) {

    P.displayZoneText(2, c_time_3, d_min_pos, 0, 0, PA_PRINT, PA_NO_EFFECT);
    P.displayAnimate();

  } else if (millis() - m < 5000) {

    P.displayZoneText(2, c_time_0, d_min_pos, 0, 0, PA_PRINT, PA_NO_EFFECT);
    P.displayAnimate();

  } else if (millis() - m < 5500) {

    P.displayZoneText(2, c_time_2, d_min_pos, 0, 0, PA_PRINT, PA_NO_EFFECT);
    P.displayAnimate();

  } else if (millis() - m < 6000) {

    P.displayZoneText(2, c_time_0, d_min_pos, 0, 0, PA_PRINT, PA_NO_EFFECT);
    P.displayAnimate();

  } else {
    m = millis();
  }
}

void timekeep() {

  if (millis() - m_time >= 1000) {
    tdelay = (millis() - m_time) - 1000;
    m_time = millis() - tdelay;
    clockSec++;

    //clockSec = clockSec+55;
    //tdelay = 1000 - (millis() - m_time);
    //tdelay = millis() - (m_time + 1000);

    //clockSec++;
    Serial.println(tdelay);
    tdelay = 0;

    Serial.print(clockHour);
    Serial.print("-");
    Serial.print(clockMin);
    Serial.print("-");
    Serial.println(clockSec);
  }
  //THIS NEXT SECTION IS TO REMIDIATE CRITICAL BUG SITUATIONS
  if (clockSec > 60) {
    clockSec = clockSec - 60;
    clockMin++;
    Serial.println("SHIT SECONDS ERROR");
    //this should never happen
  }
  if (clockHour > 23) {
    clockHour = clockHour - 23;
    t_day++;
    Serial.println("SHIT HOUR ERROR");
  }
  if (clockMin > 60) {
    clockMin = clockMin - 60;
    clockHour++;
    Serial.println("SHIT MINUTES ERROR");
  }
}


void updatetime_d() {
  Serial.println("updatetime_d");
  s_hour = String(clockHour);
  s_hour_dec = String(clockHour / 10);
  s_hour_uni = String(clockHour % 10);
  s_min = String(clockMin);
  s_min_dec = String(clockMin / 10);
  s_min_uni = String(clockMin % 10);

  if (clockMin < 10) {
    s_min_dec = "0";
  }
  if (clockHour < 10) {
    s_hour_dec = " ";
  }
  s_time_2 = " " + s_min_dec;
  s_time_2.toCharArray(c_time_2, 8);
  s_hour_uni.toCharArray(c_hour_uni, 8);
  s_hour_dec.toCharArray(c_hour_dec, 8);
  s_min_uni.toCharArray(c_min_uni, 8);
  s_min_dec.toCharArray(c_min_dec, 8);

  P.displayZoneText(1, c_min_uni, i_min_pos, 50, 0, PA_SCROLL_UP, PA_NO_EFFECT);
  P.displayZoneText(2, c_time_2, d_min_pos, 50, 0, PA_SCROLL_UP, PA_NO_EFFECT);
  //P.displayAnimate();
  while (!P.getZoneStatus(1) && !P.getZoneStatus(2)) {

    P.displayAnimate();
  }
}

void updatetime_u() {
  Serial.println("updatetime_u");
  s_hour = String(clockHour);
  s_hour_dec = String(clockHour / 10);
  s_hour_uni = String(clockHour % 10);
  s_min = String(clockMin);
  s_min_dec = String(clockMin / 10);
  s_min_uni = String(clockMin % 10);

  if (clockMin < 10) {
    s_min_dec = "0";
  }
  if (clockHour < 10) {
    s_hour_dec = " ";
  }
  s_time_2 = " " + s_min_dec;
  s_time_2.toCharArray(c_time_2, 8);
  s_hour_uni.toCharArray(c_hour_uni, 8);
  s_hour_dec.toCharArray(c_hour_dec, 8);
  s_min_uni.toCharArray(c_min_uni, 8);
  s_min_dec.toCharArray(c_min_dec, 8);

  P.displayZoneText(1, c_min_uni, i_min_pos, 50, 0, PA_SCROLL_UP, PA_NO_EFFECT);
  //P.displayAnimate();
  while (!P.getZoneStatus(1)) {
    displayTime();
    P.displayAnimate();
  }
}
void update_all() {
  s_hour = String(clockHour);
  s_hour_dec = String(clockHour / 10);
  s_hour_uni = String(clockHour % 10);
  s_min = String(clockMin);
  s_min_dec = String(clockMin / 10);
  s_min_uni = String(clockMin % 10);

  if (clockMin < 10) {
    s_min_dec = "0";
  }
  if (clockHour < 10) {
    s_hour_dec = " ";
  }


  s_time_1 = ":" + s_min_dec;
  s_time_2 = " " + s_min_dec;

  s_time_1.toCharArray(c_time_1, 8);
  s_time_2.toCharArray(c_time_2, 8);
  s_hour_uni.toCharArray(c_hour_uni, 8);
  s_hour_dec.toCharArray(c_hour_dec, 8);
  s_min_uni.toCharArray(c_min_uni, 8);
  s_min_dec.toCharArray(c_min_dec, 8);


  P.displayZoneText(1, c_min_uni, i_min_pos, 50, 0, PA_SCROLL_UP, PA_NO_EFFECT);
  P.displayZoneText(2, c_time_2, d_min_pos, 50, 0, PA_SCROLL_UP, PA_NO_EFFECT);
  P.displayZoneText(3, c_hour_uni, i_hour_pos, 50, 0, PA_SCROLL_UP, PA_NO_EFFECT);
  P.displayZoneText(4, c_hour_dec, d_hour_pos, 50, 0, PA_SCROLL_UP, PA_NO_EFFECT);
  //P.displayAnimate();
  while (!P.getZoneStatus(1) && !P.getZoneStatus(2) && !P.getZoneStatus(3) && !P.getZoneStatus(4)) {

    P.displayAnimate();
  }
  Serial.println("UPDATE ALL");
}

void updatehour_u() {
  Serial.println("updatehour_u");
  s_hour = String(clockHour);
  s_hour_dec = String(clockHour / 10);
  s_hour_uni = String(clockHour % 10);
  s_min = String(clockMin);
  s_min_dec = String(clockMin / 10);
  s_min_uni = String(clockMin % 10);

  if (clockMin < 10) {
    s_min_dec = "0";
  }
  if (clockHour < 10) {
    s_hour_dec = " ";
  }
  s_time_2 = " " + s_min_dec;
  s_time_2.toCharArray(c_time_2, 8);
  s_hour_uni.toCharArray(c_hour_uni, 8);
  s_hour_dec.toCharArray(c_hour_dec, 8);
  s_min_uni.toCharArray(c_min_uni, 8);
  s_min_dec.toCharArray(c_min_dec, 8);

  P.displayZoneText(1, c_min_uni, i_min_pos, 50, 0, PA_SCROLL_UP, PA_NO_EFFECT);
  P.displayZoneText(2, c_time_2, d_min_pos, 50, 0, PA_SCROLL_UP, PA_NO_EFFECT);
  P.displayZoneText(3, c_hour_uni, i_hour_pos, 50, 0, PA_SCROLL_UP, PA_NO_EFFECT);

  //P.displayAnimate();

  while (!P.getZoneStatus(1) && !P.getZoneStatus(2) && !P.getZoneStatus(3)) {

    P.displayAnimate();
  }
}



void updatehour_d() {
  Serial.println("updatehour_d");
  s_hour = String(clockHour);
  s_hour_dec = String(clockHour / 10);
  s_hour_uni = String(clockHour % 10);
  s_min = String(clockMin);
  s_min_dec = String(clockMin / 10);
  s_min_uni = String(clockMin % 10);

  if (clockMin < 10) {
    s_min_dec = "0";
  }
  if (clockHour < 10) {
    s_hour_dec = " ";
  }
  s_time_2 = " " + s_min_dec;
  s_time_2.toCharArray(c_time_2, 8);
  s_hour_uni.toCharArray(c_hour_uni, 8);
  s_hour_dec.toCharArray(c_hour_dec, 8);
  s_min_uni.toCharArray(c_min_uni, 8);
  s_min_dec.toCharArray(c_min_dec, 8);

  P.displayZoneText(1, c_min_uni, i_min_pos, 0, 0, PA_NO_EFFECT, PA_NO_EFFECT);
  P.displayZoneText(2, c_time_2, d_min_pos, 0, 0, PA_NO_EFFECT, PA_NO_EFFECT);
  P.displayZoneText(3, c_hour_uni, i_hour_pos, 50, 0, PA_SCROLL_UP, PA_NO_EFFECT);
  P.displayZoneText(4, c_hour_dec, d_hour_pos, 50, 0, PA_SCROLL_UP, PA_NO_EFFECT);
  //P.displayAnimate();
  while (!P.getZoneStatus(3) && !P.getZoneStatus(4)) {

    P.displayAnimate();
  }
}

void displayTime_minimalist() {
  String s_time_temp_0;
  String s_time_temp_1;
  char c_time_temp_0[16];
  char c_time_temp_1[16];

  s_hour = String(clockHour);
  //s_hour_dec = String(clockHour / 10);
  //s_hour_uni = String(clockHour % 10);
  s_min = String(clockMin);
  s_min_dec = String(clockMin / 10);
  s_min_uni = String(clockMin % 10);

  if (clockMin < 10) {
    s_min_dec = "0";
  }
  if (clockHour < 10) {
    s_hour_dec = " ";
  }

  s_time_0 = s_hour + " " + s_min_dec + s_min_uni;
  s_time_1 = s_hour + ":" + s_min_dec + s_min_uni;
  s_time_temp_0 = s_hour + " " + s_min_dec + s_min_uni + " " + String(real_t) + "ºC";
  s_time_temp_1 = s_hour + ":" + s_min_dec + s_min_uni + " " + String(real_t) + "ºC";

  s_time_0.toCharArray(c_time_0, 8);
  s_time_1.toCharArray(c_time_1, 8);
  s_time_temp_0.toCharArray(c_time_temp_0, 16);
  s_time_temp_1.toCharArray(c_time_temp_1, 16);

  if (clockMin % 20 == 0 && clockSec > 30 && clockSec < 40 && SensorEnable == true) {
    if (millis() - m < 500) {
      P.displayAnimate();
      P.displayText(c_time_temp_0, PA_RIGHT, 0, 0, PA_PRINT, PA_NO_EFFECT);
    } else if (millis() - m < 1000) {
      P.displayAnimate();
      P.displayText(c_time_temp_1, PA_RIGHT, 0, 0, PA_PRINT, PA_NO_EFFECT);
    } else {
      m = millis();
    }
  } else {
    if (millis() - m < 500) {
      P.displayAnimate();
      P.displayText(c_time_0, PA_RIGHT, 0, 0, PA_PRINT, PA_NO_EFFECT);
    } else if (millis() - m < 1000) {
      P.displayAnimate();
      P.displayText(c_time_1, PA_RIGHT, 0, 0, PA_PRINT, PA_NO_EFFECT);
    } else {
      m = millis();
    }
  }
}


void configModeCallback(WiFiManager* myWiFiManager) {
  P.displayZoneText(0, "  wifi unavailable - connect to ClockAP", PA_LEFT, 50, 1000, PA_SCROLL_RIGHT, PA_NO_EFFECT);
  P.displayAnimate();
  while (!P.getZoneStatus(0)) {
    P.displayAnimate();
    yield();
  }
}

void readSensor() {
  sensors.requestTemperatures();

  real_h = round(dht.readHumidity());
  real_t = round(dht.readTemperature());
  if (isnan(real_h) || isnan(real_t)) {
    Serial.println(F("Failed to read from DHT sensor!"));
    return;
  }
  real_hic = round(dht.computeHeatIndex(real_t, real_h, false));

  tempHumStr = String(real_h) + "% | " + String(sensors.getTempCByIndex(0)) + " ºC | " + String(real_t) + " ºC ";
  tempHumStr.toCharArray(tempHum, 64);
}


void showtemps(bool r, bool f) {

  P.setFont(ExtASCII);
  DateStr = "   " + String(t_day) + "/" + String(t_month) + " | ";
  Serial.println(DateStr);
  DateStr.toCharArray(Date, 64);

  P.displayZoneText(0, Date, PA_LEFT, 50, 250, PA_SCROLL_RIGHT, PA_NO_EFFECT);
  while (!P.getZoneStatus(0)) {

    P.displayAnimate();
    yield();
  }


  if (r == true) {
    readSensor();
    String tmp = " ";
    if (clockMin == 0) {
      tmp = "0";
      SensorReadingsArr.unshift(String(tempHum) + " " + String(DateStr) + " " + String(clockHour) + ":" + String(clockMin) + tmp);
    } else {
      SensorReadingsArr.unshift(String(tempHum) + " " + String(DateStr) + " " + String(clockHour) + ":" + String(clockMin));
    }

    Serial.println("READ SENSOR");
    Serial.println(tempHum);
    convert(tempHum, outChar);
    P.displayZoneText(0, outChar, PA_LEFT, 50, 1200, PA_SCROLL_RIGHT, PA_NO_EFFECT);
    while (!P.getZoneStatus(0)) {

      P.displayAnimate();
      yield();
    }
  }

  if (f == true) {
    if (clockHour != last_Update) {
      getWeather();
      last_Update = clockHour;
      Serial.println("WEATHER UPDATED");
    }
    Serial.println("WEATHER ");
    //Serial.println(weather_c);
    convert(weather_c, outChar);
    P.displayZoneText(0, outChar, PA_LEFT, 50, 1200, PA_SCROLL_RIGHT, PA_NO_EFFECT);
    while (!P.getZoneStatus(0)) {

      P.displayAnimate();
      yield();
    }
  }
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
}

void handleRoot() {
  unsigned long int x = millis();
  unsigned long int h = int((x / (1000 * 60 * 60)) % 24);
  unsigned long int m = int(((x / (1000 * 60)) % 60));
  unsigned long int sec = int((x / 1000) % 60);
  unsigned long int d = int((x / (1000 * 60 * 60)) / 24);

  String s = MAIN_page;  //Read HTML contents
  s = s + "<br><br>Defenições correntes:<br>RELÓGIO: " + clockHour + ":" + clockMin + " -- " + t_day + "/" + t_month + "/" + t_year + "<br><br> CIDADE: " + CIDADE + "<br>APIKEY: " + APIKEY + "<br>POSIX: " + POSIX + "<br>Idioma: " + Language + "<br>Estado do sensor: " + SensorEnable + "<br>Mudança de hora: " + CheckDST + "<br> Estado do botão:" + ButtonEnable + "<br>Mudança de hora manual: " + ClockHourOverride + "<br> luminosidade: " + intensity + "<br> Modo Nocturno: " + Minimal_enable + " Horas: De: " + Start_m_clock + "h às " + Stop_m_clock + "h" + "<br> Desligar ecrã durante a noite: " + NightmodeScreenOff + "<br> Frequência de sincronização (dias): " + Syncday + "<br> Tipo de letra: " + Font + "<br> Versão de Software: " + s_version + "<br> Tempo desde o ultimo reset: " + d + "d " + h + "h " + m + "m " + sec + " s" + "<br> Data da Ultima Sincronização: " + String(lastsyncdate);

  s = s + "</body></html>";
  server.send(200, "text/html", s);  //Send web page
}

void SensorReadingsPage() {
  String r = "<!DOCTYPE html><html><meta charset=\"utf-8\"><body><br>Leituras do Sensor:<br>";
  using index_t = decltype(SensorReadingsArr)::index_t;
  for (index_t i = 0; i < SensorReadingsArr.size(); i++) {
    r = r + "<br>" + SensorReadingsArr[i] + "<br>";
  }
  r = r + "<br> Joao Fernandes 2021</body></html>";
  r += "<br><a href='/'> Go Back </a>";
  server.send(200, "text/html", r);  //Send web page
}

void JsonSensorData() {
  String json;
  real_h = round(dht.readHumidity());
  real_t = round(dht.readTemperature());
  sensors.requestTemperatures();


  json = "{\"Sensor1_Temperature\": " + String(sensors.getTempCByIndex(0));
  json = json + ",\"Sensor2_Temperature\": " + String(real_t);
  json = json + ",\"Sensor2_Humidity\": " + String(real_h) + "}";

  server.send(200, "application/json", json);
}


void deleteall() {
  String r = "<!DOCTYPE html><html><meta charset=\"utf-8\"><body><br>ISTO IRÁ APAGAR TODAS AS DEFINIÇÕES DO RELÓGIO E EM 5 SEGUNDOS REINICIAR O DISPOSITIVO<br></body></html>";
  server.send(200, "text/html", r);  //Send web page

  for (int i = 0; i < EEPROM.length(); i++) {
    EEPROM.write(i, 0);
  }
  EEPROM.commit();
  Serial.println("SETTINGS ERASED RESET IN 5 SECONDS");
  delay(5000);
  ESP.reset();
}
void turnOnOffscreen() {
  String r;
  if (screenstate == true) {
    screenstate = false;
    P.displayShutdown(1);
    Serial.println("SCREEN OFF");
    String r = "<!DOCTYPE html><html><meta charset=\"utf-8\"><body><br>SCREEN OFF<br><a href='/turnOnOffscreen'> TURN ON </a><br><a href='/'>BACK</a></body></html>";
    r = r + "<br><a href='/'> Go Back </a>";
    server.send(200, "text/html", r);  //Send web page
  } else {
    P.displayShutdown(0);
    Serial.println("SCREEN ON");
    screenstate = true;
    String r = "<!DOCTYPE html><html><meta charset=\"utf-8\"><body><br>SCREEN ON<br><a href='/turnOnOffscreen'> TURN OFF </a><br><a href='/'>BACK</a></body></html>";
    r = r + "<br><a href='/'> Go Back </a>";
    server.send(200, "text/html", r);  //Send web page
  }
}
void reboot() {
  String r = "<!DOCTYPE html><html><meta charset=\"utf-8\"><body><br>Reinicio em 2s<br></body></html>";
  server.send(200, "text/html", r);  //Send web page
  delay(2500);
  ESP.reset();
}
void handleForm() {
  short int tmp = 0;
  short int tmp_s = 0;
  short int vcheck = 0;
  short int tmp2 = 0;
  String tmp_str;
  bool writeEEPROM = false;
  bool cityresize = false;
  tmp_str = server.arg("CIDADE");
  tmp_str.trim();
  variable_len = tmp_str.length();
  tmp2 = EEPROM.read(tmp);
  Serial.print("VARIABLE LEN: ");
  Serial.println(variable_len);
  if (variable_len == 0) {
    vcheck = EEPROM.read(tmp);
    if (vcheck == 0) {
      String s = "<a href='/'> NOME DA CIDADE NÃO FOI CONFIGURADO </a>";
      server.send(200, "text/html", s);  //Send web page
    }
    tmp = tmp + vcheck + 1;
  }

  else {
    CIDADE = server.arg("CIDADE");
    CIDADE.trim();
    CIDADE.replace(" ", "-");
    Serial.print("\nCIDADE:");
    Serial.print(CIDADE);
    Serial.print("\nCIDADE LENGHT:");
    Serial.println(variable_len);
    EEPROM.write(tmp, variable_len);
    writeEEPROM = true;
    tmp++;
    //EEPROM.commit();
    for (int i = tmp; i < (variable_len + tmp); i++) {

      EEPROM.write(i, CIDADE[i - tmp]);
      Serial.println(CIDADE[i - tmp]);
    }
    tmp = variable_len + tmp;
    //EEPROM.commit();
    tmp_s = tmp;
    Serial.print("TMP_S_BEFORE_RELLOCATING_VARIABLES: ");
    Serial.println(tmp);
    if (tmp2 != CIDADE.length()) {
      cityresize = true;
      Serial.println("TMP_S: ");
      Serial.print(tmp_s);
      Serial.println("RELOCATING VARIABLES");
      Serial.print("\nAPIKEY:");
      Serial.print(APIKEY);
      Serial.print("\nAPIKEY LENGHT:");
      Serial.println(APIKEY.length());
      EEPROM.write(tmp_s, APIKEY.length());
      tmp_s++;
      EEPROM.commit();
      for (int i = tmp_s; i < (APIKEY.length() + tmp_s); i++) {

        EEPROM.write(i, APIKEY[i - tmp_s]);
        Serial.println(APIKEY[i - tmp_s]);
      }
      // EEPROM.commit();

      tmp_s = tmp_s + APIKEY.length();
      Serial.print("TMP_S_afterAPI: ");
      Serial.println(tmp_s);
      EEPROM.commit();  //END APIKEY


      Serial.print("\nSETPOSIX:");
      Serial.print(POSIX);
      Serial.print("\nSETPOSIX_len: ");
      Serial.println(POSIX.length());
      EEPROM.write(tmp_s, POSIX.length());
      tmp_s++;

      Serial.print("TMP_S_afterSetPosix: ");
      Serial.println(tmp_s);
      EEPROM.commit();
      for (int i = tmp_s; i < (POSIX.length() + tmp_s); i++) {

        EEPROM.write(i, POSIX[i - tmp_s]);
        Serial.println(POSIX[i - tmp_s]);
      }
      EEPROM.commit();

      tmp_s = variable_len + tmp_s;
      //tmp = tmp_s;
      Serial.println("TMP_S_ENDPOSIX: ");
      Serial.print(tmp_s);
    }
  }
  Serial.print("TMP_BEFORE_NORMAL_API_CHECK: ");
  Serial.println(tmp);
  tmp_str = server.arg("APIKEY");
  tmp_str.trim();
  variable_len = tmp_str.length();
  if (variable_len == 0) {
    vcheck = EEPROM.read(tmp);
    if (vcheck == 0) {
      String s = "<a href='/'> API KEY NÃO FOI CONFIGURADO </a>";
      server.send(200, "text/html", s);  //Send web page
    }
    tmp = tmp + vcheck + 1;
  } else {
    APIKEY = server.arg("APIKEY");
    APIKEY.trim();
    Serial.print("\nAPIKEY:");
    Serial.print(APIKEY);
    Serial.print("\nAPIKEY LENGHT:");
    Serial.println(variable_len);
    EEPROM.write(tmp, variable_len);
    tmp++;
    //EEPROM.commit();
    for (int i = tmp; i < (variable_len + tmp); i++) {

      EEPROM.write(i, APIKEY[i - tmp]);
      Serial.println(APIKEY[i - tmp]);
    }
    //EEPROM.commit();

    tmp = tmp + variable_len;
    //EEPROM.commit();
    writeEEPROM = true;
  }
  Serial.print("TMP_AFTER_NORMAL_api_CHECK: ");
  Serial.println(tmp);
  tmp_str = server.arg("SETPOSIX");
  tmp_str.trim();
  variable_len = tmp_str.length();
  Serial.print("\nPOSIX LENGHT:");
  Serial.println(variable_len);
  if (variable_len == 0) {
    vcheck = EEPROM.read(tmp);
    Serial.print("vcheck_POSIX: ");
    Serial.println(vcheck);

    if (server.hasArg("DSTEnable")) {
      if (server.arg("DSTEnable)==1")) {

        if (vcheck == 0) {
          String s = "<a href='/'> POSIX NAO FOI CONFIGURADO </a>";
          server.send(200, "text/html", s);  //Send web page
        }
      }
    }
    tmp = tmp + vcheck + 1;
  } else {
    POSIX = server.arg("SETPOSIX");
    POSIX.trim();
    Serial.print("\nSETPOSIX:");
    Serial.print(POSIX);
    Serial.print("\nSETPOSIX_len: ");
    Serial.println(variable_len);
    EEPROM.write(tmp, variable_len);
    tmp++;
    //EEPROM.commit();
    for (int i = tmp; i < (variable_len + tmp); i++) {

      EEPROM.write(i, POSIX[i - tmp]);
      Serial.println(POSIX[i - tmp]);
    }
    //EEPROM.commit();

    tmp = variable_len + tmp;
    writeEEPROM = true;
  }
  Serial.print("TMP_AFTER_POSIX_CHECK: ");
  Serial.println(tmp);
  Serial.println("LANGUAGE");
  tmp_str = server.arg("Language");
  tmp_str.trim();
  variable_len = tmp_str.length();
  Serial.print("Lang len: ");
  Serial.println(variable_len);

  if (variable_len != 0) {
    if (EEPROM.read(tmp) != 2) {
      variable_len = 2;
      EEPROM.write(tmp, 2);
      //EEPROM.commit();
      writeEEPROM = true;
    }
    tmp++;


    if (Language == server.arg("Language") && cityresize == false) {
      Serial.println("\n Ignoring language setting === Already set");
      //tmp = tmp + 2 ;
    } else {
      Language = tmp_str;
      for (int i = tmp; i < tmp + variable_len; i++) {

        EEPROM.write(i, tmp_str[i - tmp]);
        Serial.println(tmp_str[i - tmp]);
      }
      //EEPROM.commit();
      writeEEPROM = true;
    }
    tmp = tmp + variable_len;
    //writeEEPROM = true;
  } else {
    Serial.println("\n Ignoring language setting 0 length");
    if (EEPROM.read(tmp) != 2) {
      Serial.println("LANGUAGE SET TO DEFAULT PT");
      EEPROM.write(tmp, 2);
      tmp++;
      EEPROM.write(tmp, 'p');
      tmp++;
      EEPROM.write(tmp, 't');
      tmp++;
      //EEPROM.commit();


      writeEEPROM = true;
    } else {
      tmp = tmp + 3;
    }
  }

  if (server.hasArg("SensorEnable")) {
    if (server.arg("SensorEnable)==1")) {
      if (EEPROM.read(tmp) != 1) {
        Serial.println("SENSOR ENABLED");
        Serial.print("sensor enable location: ");
        Serial.println(tmp);
        EEPROM.write(tmp, 1);
        //EEPROM.commit();
        writeEEPROM = true;
      } else {
        Serial.println("Sensor already set Skipping");
      }
    }
  } else {
    if (EEPROM.read(tmp) != 0) {
      Serial.println("SENSOR DISABLED");
      EEPROM.write(tmp, 0);
      //EEPROM.commit();
      writeEEPROM = true;
    } else {
      Serial.println("Sensor already set Skipping");
    }
  }
  tmp++;

  if (server.hasArg("DSTEnable")) {
    if (server.arg("DSTEnable)==1")) {
      if (EEPROM.read(tmp) != 1) {
        Serial.println("DST ENABLED");
        EEPROM.write(tmp, 1);
        Serial.print("DST enable location: ");
        Serial.println(tmp);
        //EEPROM.commit();
        writeEEPROM = true;
      } else {
        Serial.println("DST already set Skipping");
      }
    }

  } else {
    if (EEPROM.read(tmp) != 0) {

      Serial.println("DST DISABLED");
      EEPROM.write(tmp, 0);
      //EEPROM.commit();
      writeEEPROM = true;
    } else {
      Serial.println("DST already set Skipping");
    }
  }

  tmp++;
  if (server.hasArg("BTNEnable")) {
    if (server.arg("BTNEnable)==1")) {
      if (EEPROM.read(tmp) != 1) {
        Serial.println("BTN ENABLED");
        Serial.print("BTN enable location: ");
        Serial.println(tmp);
        EEPROM.write(tmp, 1);
        writeEEPROM = true;
        //EEPROM.commit();
      } else {
        Serial.println("BTN already set Skipping");
      }
    }
  } else {
    if (EEPROM.read(tmp) != 0) {

      Serial.println("BTN DISABLED");
      EEPROM.write(tmp, 0);
      writeEEPROM = true;
      //EEPROM.commit();
    } else {
      Serial.println("BTN already set Skipping");
    }
  }
  tmp++;
  if (ClockHourOverride == server.arg("ClockHourOverride").toInt() && cityresize == false) {
    Serial.println("ClockHourOverride same as it was set Skipping");
  } else {
    ClockHourOverride = server.arg("ClockHourOverride").toInt();
    ClockHourOverride = constrain(ClockHourOverride, -12, 12);
    if (ClockHourOverride < 0) {
      ClockHourOverride = abs(ClockHourOverride) + 12;
    }
    if (EEPROM.read(tmp) != ClockHourOverride) {
      Serial.println("ClockHourOverride");
      Serial.println(ClockHourOverride);
      Serial.print("CLOCKOVERRIDE location: ");
      Serial.println(tmp);
      EEPROM.write(tmp, ClockHourOverride);
      //EEPROM.commit();
      Serial.print(tmp);
      Serial.print("-");
      Serial.println(EEPROM.read(tmp));
      writeEEPROM = true;
    } else {
      Serial.println("ClockHourOverride already set Skipping");
    }
  }
  tmp++;

  if (intensity == server.arg("intensity").toInt() && cityresize == false) {
    Serial.println("Intensity already setup Skip: ");
  } else {
    intensity = server.arg("intensity").toInt();
    intensity = constrain(intensity, 0, 15);
    if (EEPROM.read(tmp) == intensity) {
      Serial.println("Intensity already setup Skip: ");
      Serial.print(EEPROM.read(tmp));
    } else {
      Serial.println("intensity");
      Serial.println(intensity);
      Serial.print("INTENSITY location: ");
      Serial.println(tmp);
      EEPROM.write(tmp, intensity);
      writeEEPROM = true;
      //EEPROM.commit();
      Serial.print(tmp);
      Serial.print("-");
      Serial.println(EEPROM.read(tmp));
      P.setIntensity(intensity);
    }
  }
  tmp++;

  if (server.hasArg("NightModeEnable")) {
    if (server.arg("NightModeEnable)==1")) {
      if (EEPROM.read(tmp) != 1) {

        Serial.println("NightMode Enable");
        EEPROM.write(tmp, 1);
        Serial.print("NightMode Enable location: ");
        Serial.println(tmp);
        //EEPROM.commit();
        Minimal_enable = true;
        writeEEPROM = true;
      } else {
        Serial.println("Nightmode Already Set Skipping");
      }
    }

  } else {
    if (EEPROM.read(tmp) != 0) {

      Serial.println("NightMode DISABLED");
      EEPROM.write(tmp, 0);
      //EEPROM.commit();
      Minimal_enable = false;
      writeEEPROM = true;
    } else {
      Serial.println("Nightmode Already Set Skipping");
    }
  }

  tmp++;
  if (server.arg("StartNightMode").toInt() != server.arg("StopNightMode").toInt() && cityresize == false) {
    Start_m_clock = server.arg("StartNightMode").toInt();
    Start_m_clock = constrain(Start_m_clock, 0, 23);
    Serial.println("Start_m_clock");
    Serial.println(Start_m_clock);
    Serial.print("Start_m_clock location: ");
    Serial.println(tmp);
    EEPROM.write(tmp, Start_m_clock);
    //EEPROM.commit();
    Serial.print(tmp);
    Serial.print("-");
    Serial.println(EEPROM.read(tmp));

    tmp++;

    Stop_m_clock = server.arg("StopNightMode").toInt();
    Stop_m_clock = constrain(Stop_m_clock, 0, 23);
    Serial.println("Stop_m_clock");
    Serial.println(Stop_m_clock);
    Serial.print("Stop_m_clock location: ");
    Serial.println(tmp);
    EEPROM.write(tmp, Stop_m_clock);
    //EEPROM.commit();
    Serial.print(tmp);
    Serial.print("-");
    Serial.println(EEPROM.read(tmp));
    tmp++;
    writeEEPROM = true;
  } else {
    if (EEPROM.read(tmp) == EEPROM.read(tmp + 1) || EEPROM.read(tmp) > 23 || EEPROM.read(tmp + 1) > 23) {
      if (Start_m_clock != Stop_m_clock) {
        EEPROM.write(tmp, Start_m_clock);
        tmp++;
        EEPROM.write(tmp, Stop_m_clock);
        tmp++;
        writeEEPROM = true;
        Serial.println("WRITING ENFORCEMENT NIGHTTIME VARIABLES FROM MEMORY");
      } else {

        Serial.println("\nENABLING DEFAULT NIGHT TIME");
        EEPROM.write(tmp, 5);
        Start_m_clock = 5;
        //EEPROM.commit();
        tmp++;
        EEPROM.write(tmp, 6);
        Stop_m_clock = 6;
        //EEPROM.commit();
        tmp++;
        writeEEPROM = true;
      }
    } else {
      if (EEPROM.read(tmp) != Start_m_clock || EEPROM.read(tmp + 1) != Stop_m_clock) {
        EEPROM.write(tmp, Start_m_clock);
        tmp++;
        EEPROM.write(tmp, Stop_m_clock);
        tmp++;
        writeEEPROM = true;
        Serial.println("WRITING ENFORCEMENT NIGHTTIME VARIABLES FROM MEMORY");
      } else {
        Serial.println("\nIGNORING NIGHTIME VARIABLES");
        tmp++;
        tmp++;
      }
    }
  }

  if (server.hasArg("NightmodeScreenOff")) {
    if (server.arg("NightmodeScreenOff)==1")) {
      if (EEPROM.read(tmp) != 1) {
        EEPROM.write(tmp, 1);
        //EEPROM.commit();
        NightmodeScreenOff = true;
        Serial.println("SETTING NIGHTSCREEN OFF ENABLED");
        writeEEPROM = true;

      } else {
        Serial.println("NightmodeScreenOff already set, skipping");
        NightmodeScreenOff = true;
      }
    }
  } else {
    if (EEPROM.read(tmp) != 0) {
      EEPROM.write(tmp, 0);
      //EEPROM.commit();
      NightmodeScreenOff = false;
      Serial.println("SETTING NIGHTSCREEN OFF DISBLED");
      writeEEPROM = true;
    } else {
      Serial.println("NightmodeScreenOff already set, skipping");
      NightmodeScreenOff = false;
    }
  }

  tmp++;
  Serial.print("SYNCDAY TMP LOCATION:");
  Serial.println(tmp);
  if ((Syncday == server.arg("SyncDay").toInt() || server.arg("SyncDay").toInt() == 0) && cityresize == false) {
    Serial.println("Frequency Sync same as before skipping");
  } else {
    if (server.arg("SyncDay").toInt() == 0) {
      if (EEPROM.read(tmp) == Syncday) {
        Serial.println("Frequency Sync Already as was, skipping");
      } else {
        //Syncday = constrain(Syncday, 2, 30);
        EEPROM.write(tmp, Syncday);
        //EEPROM.commit();
        writeEEPROM = true;
        Serial.println("ENFORCING SYNCDAY SET -- DIFFERENT THAN RECORDED");
      }
    } else {
      Syncday = server.arg("SyncDay").toInt();

      if (EEPROM.read(tmp) != Syncday) {
        Syncday = constrain(Syncday, 2, 30);
        EEPROM.write(tmp, Syncday);
        //EEPROM.commit();
        writeEEPROM = true;
      } else {
        Serial.println("Frequency Sync Already set, skipping");
      }
    }
  }
  tmp++;

  Serial.print("FONT TMP LOCATION:");
  Serial.println(tmp);
  if (Font == server.arg("Font").toInt() || server.arg("Font").toInt() == 0 && cityresize == false) {
    Serial.println("Font same as it was set, skipping");
  } else {
    if (server.arg("Font").toInt() == 0 && EEPROM.read(tmp) != Font) {
      Serial.println("FONT was not changed, writing same font");
      EEPROM.write(tmp, Font);
      writeEEPROM = true;
    } else {
      Font = server.arg("Font").toInt();
      Serial.print("FONT: ");
      Serial.println(Font);

      if (Font > 4) {
        Font = 1;  //DEFAULT
        Serial.println("DEFAULT FONT SET Err: >2");
      }
      if (EEPROM.read(tmp) != Font) {
        EEPROM.write(tmp, Font);
        //EEPROM.commit();
        Serial.println("Font set");
        writeEEPROM = true;
      } else {
        Serial.println("Font Already set, skipping");
      }
    }
  }
  tmp++;


  if (writeEEPROM == true) {
    for (int i = tmp; i < 199; i++) {
      EEPROM.write(i, 'd');
    }
    Serial.println("WRITING TO MEMORY\n");
    EEPROM.commit();
  } else {
    Serial.println("NOTHING TO WRITE\n");
  }
  for (int i = 0; i < 150; i++) {
    char a = char(EEPROM.read(i));
    Serial.print(a);
  }



  if (server.hasArg("RESET")) {
    if (server.arg("RESET)==1")) {
      Serial.println("RESET");
      String s = "<br> RESET <br>";
      server.send(200, "text/html", s);  //Send web page
      delay(1000);
      ESP.reset();
    }
  } else {
    Serial.println("\nRESET DISABLED");
  }


  String s = "<a href='/'> Go Back </a>";
  server.send(200, "text/html", s);  //Send web page

  loop();
}
