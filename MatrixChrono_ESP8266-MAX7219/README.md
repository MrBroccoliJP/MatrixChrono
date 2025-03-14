# MatrixChrono - IoT Smart Home Clock System

<img src="MatrixChrono_ESP8266-MAX7219_Images/logo.gif" alt="drawing" width="600"/>

## MatrixChrono_ESP32-NeoMatrix

<img src="MatrixChrono_ESP8266-MAX7219_Images/MatrixChrono_ESP8266-MAX7219_Image.JPG" alt="drawing" width="400"/>

-   **Microcontroller**: ESP8266
-   **Display**: MAX7219 4x 8x8 LED Matrix 
-   **Sensors**:
    -   🌡️ DHT11 (for humidity and temperature)
    -   🌡️ DS18B20 for more accurate temperature readings
-   **Features**:
    -   🕒 Custom timekeeping function
    -   🌙 Night mode with reduced brightness
    -   💾 EEPROM-based persistent storage
    -   🏠Home Assistant JSON output  

-----

## 🔨 **[This Page is a Work in Progress]**  🔨

---

### ⚙️ **Hardware Requirements**

- **Power Supply (PSU)**  
  - **1x 5V PSU**  

- **Core Components**  
  - **1x ESP8266 Dev Board**  (WEMOS D1 MINI SIZE)
  - **1x DHT11 Temperature and Humidity Sensor**    
  - **1x DS18B20 Temperature Sensor**  
  - **4x MAX7219 controlled 8x8 LED BOARDS**  
  - **1x Push Button**  

- **Optional Components**  
  - **1x Power Filter Board**  
  - **1x Step-Down Converter** (12V → 5V)

----

### ⚡ **Wiring Diagram**  
<img src="MatrixChrono_ESP8266-MAX7219_Images/Wiring diagram.png" alt="drawing" width="1000"/>

-----

### 🧑‍💻 **Instructions**

If you'd like to recreate this project, feel free to do so and contribute! This is a personal project, and while I appreciate any contributions, I can't guarantee long-term maintenance.

I'll provide the code and general instructions, but I won't be offering in-depth tech support. This is a DIY project.

### Setup Instructions

1. **Connect the Modules**

   Follow the wiring diagram to set up the hardware.

   **Important Notes:**
   - **Stable Voltage Supply**: Ensure the modules receives clean, stable voltage.
   - **Common Grounding**: All modules must share a common ground.
   - **Signal Integrity**:
     - Use a shielded cable for the screens and sensor signal wires OR keep the cables short.
     - Avoid routing the signal wire near antennas, coils, or other interference-prone components.  
   🚨 **Failure to follow these steps may cause erratic behavior**—such as flickering, bad sensor readings or other malfunctions.
   
2. **Library/Boards Manager Versions:**
Make sure you set your libraries to the following versions. (Libraries without version-specific requirements are omitted.)
- [DallasTemperature](https://github.com/milesburton/Arduino-Temperature-Control-Library): V3.9.0 [TODO: Update sketch to comply with the library changes]
  


1. **Upload the Sketch to the ESP8266**
   - Connect the ESP8266 to your computer via USB and verify it appears as a serial device.
   - Create a `Secrets.h` file and add your API key for WeatherAPI:  
     `String APIKEY = "YOUR_API_KEY";`

2. **Power Up**

   - Remove the USB connection and connect the PSU power.  

-----

## 🛠️ **Initial Setup**

  -   **Connect to the Wi-Fi portal for initial Wi-Fi setup:** [WifiManager](https://github.com/tzapu/WiFiManager)
        - Connect to the Wi-Fi SSID shown in the screen of the module: "CLOCK_AP"
        - Access the IP: `192.168.1.4`
        - Connect to your Wi-Fi with your password
        - The board will reset, and after a successful connection, the clock will display.

-   **Access the web interface to customize settings:**
    - Set up the [POSIX](https://github.com/ropg/ezTime?tab=readme-ov-file#timezones-1) according to your timezone.  
    - Set up the [API KEY](https://weatherapi.com) for weatherapi.com.  
    - Customize other settings according to your preferences.


--------
## 📡 API & Home Automation Integration (HomeAssistant)

MatrixChrono provides a JSON output of sensor data, making it compatible with **Home Assistant**.  
https://www.home-assistant.io/  


<img src="MatrixChrono_ESP8266-MAX7219_Images/Home_Assistant_data_integration.png" alt="drawing" width="1000"/>


Configuration.yaml example:
    
*`/homeassistant/configuration.yaml`*


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