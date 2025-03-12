# MatrixChrono - IoT Smart Home Clock System

**MatrixChrono** is a collection of IoT-enabled smart home clock systems featuring real-time weather updates, sensor feedback, and home automation integration. Using matrix-based LED displays, it offers a sleek and functional way to track time, temperature, and more. The project supports two hardware configurations: one with ESP8266 and MAX7219 displays, and another with ESP32 and a NeoPixel matrix. 

## 🚀 Features

-   🌡️ **Sensor Integration**: Supports DS18B20, DHT11 for temperature/humidity sensing.

-   ⏰ **Automatic Time Updates**: Syncs time via NTP, automatic DST adjustment.

-   🌎 **Weather Updates**: Retrieves weather data hourly from WeatherAPI.com.

-   📡 **Wi-Fi Configuration**: Uses WiFiManager for easy network setup.

-   🎨 **Customizable Display**: Color selection, multiple fonts, and night mode.

-   📊 **Web Interface**: HTML + CSS-based configuration page with a temperature graph.

-   🔄 **OTA Updates**: Firmware updates over-the-air for seamless upgrades.

-   🏡 **Home Assistant Integration**: JSON sensor output for smart home automation.
----------

## 🔧 Hardware Variants

### **MatrixChrono_ESP32-NeoMatrix**

-   **Microcontroller**: ESP32
    
-   **Display**: NeoPixel Matrix [256 leds]
    
-   **Sensors**:
    
    -   DS18B20 Temperature Sensor
        
    -   RTC DS3231 for precise timekeeping
        
-   **Features**:
    
    -   Configurable web pages (forms, color wheel, graph plotting)
        
    -   Night mode with brightness adjustments
        
    -   EEPROM-based persistent storage
        
    -   Wi-Fi portal for configuration
        

### **MatrixChrono_ESP8266-MAX7219**

-   **Microcontroller**: ESP8266
    
-   **Display**: MAX7219 4x8 LED Matrix
    
-   **Sensors**:
    
    -   DHT11 (for humidity and temperature) _(Optional: DS18B20 for better accuracy)_
        
-   **Features**:
    
    -   Custom timekeeping function
        
    -   Night mode with reduced brightness
        
    -   EEPROM-based persistent storage
        
    -   Home Assistant JSON output
        

----------

## 🛠️ Tech Stack

-   **Firmware**: C, C++, Arduino
    
-   **Hardware**: ESP32, ESP8266, MAX7219, NeoPixel, Sensors (DS18B20, DHT11, RTC DS3231)
    
-   **Web Interface**: HTML, CSS
    
-   **Networking**: WiFi, NTP

----------
## 📚 Libraries Used

> *"If I have seen further, it is by standing on the shoulders of
> giants."* — Sir Isaac Newton

### **MatrixChrono_ESP32-NeoMatrix**

-   [WiFiManager](https://github.com/tzapu/WiFiManager)
    
-   [WebServer](https://github.com/espressif/arduino-esp32/tree/master/libraries/WebServer)
    
-   [WiFiClient](https://github.com/espressif/arduino-esp32/tree/master/libraries/WiFi)
    
-   [CircularBuffer](https://github.com/rlogiacco/CircularBuffer)
    
-   [EEPROM](https://github.com/espressif/arduino-esp32/tree/master/libraries/EEPROM)
    
-   [ArduinoOTA](https://github.com/esp8266/Arduino/tree/master/libraries/ArduinoOTA)
    
-   [Adafruit GFX](https://github.com/adafruit/Adafruit-GFX-Library)
    
-   [Adafruit NeoMatrix](https://github.com/adafruit/Adafruit_NeoMatrix)
    
-   [Adafruit NeoPixel](https://github.com/adafruit/Adafruit_NeoPixel)
    
-   [ArduinoJson](https://arduinojson.org/)
    
-   [RTClib](https://github.com/adafruit/RTClib)
    
-   [ezTime](https://github.com/ropg/ezTime)
    
-   [DallasTemperature](https://github.com/milesburton/Arduino-Temperature-Control-Library)
    

### **MatrixChrono_ESP8266-MAX7219**

-   [ArduinoOTA](https://github.com/esp8266/Arduino/tree/master/libraries/ArduinoOTA)
    
-   [ESP8266HTTPClient](https://github.com/esp8266/Arduino/tree/master/libraries/ESP8266HTTPClient)
    
-   [WiFiClient](https://github.com/esp8266/Arduino/tree/master/libraries/ESP8266WiFi)
    
-   [NTPClient](https://github.com/arduino-libraries/NTPClient)
    
-   [WiFiManager](https://github.com/tzapu/WiFiManager)
    
-   [ESP8266mDNS](https://github.com/esp8266/Arduino/tree/master/libraries/ESP8266mDNS)
    
-   [ArduinoJson](https://arduinojson.org/)
    
-   [MD_Parola](https://github.com/MajicDesigns/MD_Parola)
    
-   [MD_MAX72xx](https://github.com/MajicDesigns/MD_MAX72XX)
    
-   [DHT Sensor Library](https://github.com/adafruit/DHT-sensor-library)
    
-   [DallasTemperature](https://github.com/milesburton/Arduino-Temperature-Control-Library)
    
-   [ezTime](https://github.com/ropg/ezTime)

------
## 📜 Installation & Setup

If you’d like to recreate this project, feel free to do so and contribute! This is a personal project, and while I appreciate any contributions, I can’t promise to maintain it indefinitely. While I provide the code and general instructions, I won't be providing in depth tech support. DIY project.

### Requirements
 - Choose a specific config: **MatrixChrono_ESP32-NeoMatrix** OR **MatrixChrono_ESP8266-MAX7219**

 - Read the documentation inside each for specific instructions.

### Configuration

	-   Connect to the WiFi portal for initial setup.
    
	-   Access the web interface to customize settings.
    
	-   View real-time temperature graphs and weather data.

------

## 📡 API & Home Automation Integration

MatrixChrono provides a JSON output of sensor data, making it compatible with **Home Assistant**.
https://www.home-assistant.io/
My config on *`/homeassistant/configuration.yaml`*


For ***MatrixChrono_ESP32-NeoMatrix***

    sensor:
      - platform: rest
        name: "MatrixChrono_ESP32-NeoMatrix Temperature"
        unit_of_measurement: "°C"
        resource: "**MATRIXCHRONO_IP_ADDRESS**/JsonSensorData"
        scan_interval: 900
        value_template: "{{ value_json.Temperature }}"
        device_class: temperature
        unique_id: "UNIQUEID"


For ***MatrixChrono_ESP8266-MAX7219***

    sensor:
      - platform: rest
        name: "MatrixChrono_ESP8266-MAX7219 Sensor1 Temperature"
        unit_of_measurement: "°C"
        resource: "**MATRIXCHRONO_IP_ADDRESS**/JsonSensorData"
        scan_interval: 900
        value_template: "{{ value_json.Sensor1_Temperature }}"
        device_class: temperature
        unique_id: "UNIQUEID"
    
      - platform: rest
        name: "MatrixChrono_ESP8266-MAX7219 Sensor2 Temperature"
        unit_of_measurement: "°C"
        resource: "**MATRIXCHRONO_IP_ADDRESS**/JsonSensorData"
        scan_interval: 900
        value_template: "{{ value_json.Sensor2_Temperature }}"
        device_class: temperature
        unique_id: "UNIQUEID"
    
      - platform: rest
        name: "MatrixChrono_ESP8266-MAX7219 Sensor2 Humidity"
        unit_of_measurement: "%"
        resource: "**MATRIXCHRONO_IP_ADDRESS**/JsonSensorData"
        scan_interval: 900
        value_template: "{{ value_json.Sensor2_Humidity }}"
        device_class: humidity
        unique_id: "UNIQUEID"


Make sure you change **MATRIXCHRONO_IP_ADDRESS** and **UNIQUEID** depending on your settings. 

## 🔧 General Troubleshooting
**Sketch not uploading?** Verify library versions
**Sketch uploaded but nothing is displayed?** Check wiring, or library versions, verify log output on serial interface
**Weather not displaying?** Ensure API key is correctly configured.

