#include <Adafruit_Si7021.h>
#include <ESP8266WiFi.h>
#include <stdio.h>
#include <math.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>  // Requires version 5
#include "config.h" // Contains WLAN_SSID and WLAN_PASS defines
#include <weather_station_lib.h>
#include <Wire.h>

// API parameters
#define WIFI_RETRY_DELAY 500
#define MAX_WIFI_INIT_RETRY 50
#define REST_ENDPOINT "192.168.0.171:8080/rest"
#define LOCATION "Livingroom"

#define FILTER_COUNT 50 // amount of samples to take for battery voltage filtering
#define EMPIRICAL_VOLTAGE_OFFSET 0.0

#define SLEEP_TIME_MIN 5 

// Create struct for data
Scout scout(0,0,0,0);

float analogVals[FILTER_COUNT] = {0};

bool enableHeater = false;
// Create instance of si7021 object from adafruit library
Adafruit_Si7021 sensor = Adafruit_Si7021();

WiFiClient client;
HTTPClient http;

void setup() {
  Serial.begin(115200);
  Serial.println();

  init_sensor();
  
  if (init_wifi() == WL_CONNECTED) {
    Serial.print("Connected to ");
    Serial.print(WLAN_SSID);
    Serial.print(" --- IP: ");
    Serial.println(WiFi.localIP());
  }
  else {
    Serial.print("Error connecting to: ");
    Serial.print(WLAN_SSID);
  }

  if ((WiFi.status() == WL_CONNECTED)) {
    getBatteryStatus();
    getWeather();

    char temp_item[30];
    char hum_item[30];
    char voltage_item[30];
    char percent_item[30];
    sprintf(temp_item, "%s_temp", LOCATION);
    sprintf(hum_item, "%s_hum", LOCATION);
    sprintf(voltage_item, "%s_bat_voltage", LOCATION);
    sprintf(percent_item, "%s__bat_percent", LOCATION);

    char url[70];
    char value[15];

    const char* items[4] = {temp_item, hum_item, voltage_item, percent_item};
    
    // Post all fields of struct
    for (int i = 0; i < 4; i++) {
      sprintf(url, "http://%s/items/%s", REST_ENDPOINT, items[i]);
      http.begin(client, url);
      sprintf(value, "%f", scout.get_value(i));
      http.addHeader("Content-Type", "text/plain");
      int httpCode = http.POST(value);
      httpHandleReturn(httpCode);
      http.end();
    }

    // sprintf(battery_url, "http://%s/battery/%s", REST_ENDPOINT, LOCATION);
    // sprintf(battery_payload, "{\"voltage\": \"%f\", \"percent_charge\": \"%f\"}", voltage, percent_charge);

    // sprintf(telemetry_url,  "http://%s/telemetry/%s", REST_ENDPOINT, LOCATION);
    // sprintf(telemetry_payload, "{\"Voltage offset\": \"%f\", \"Filter count\": \"%d\", \"Server IP\": \"%s\", \"Sleep time\": \"%d min\", \"location\": \"%s\"}", EMPIRICAL_VOLTAGE_OFFSET, FILTER_COUNT, SERVER_IP, SLEEP_TIME_MIN, LOCATION);
  }
Serial.printf("going into Deep Sleep for %d minutes...", SLEEP_TIME_MIN);
ESP.deepSleep(SLEEP_TIME_MIN * 60E6);
}

void loop() {
}

//---------------------------------------------------------------
int init_wifi() {
  int retries = 0;
  
  Serial.println("\nConnecting to WiFi AP..........");
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(WLAN_SSID, WLAN_PASS);
  // check the status of WiFi connection to be WL_CONNECTED
  while ((WiFi.status() != WL_CONNECTED) && (retries < MAX_WIFI_INIT_RETRY)) {
      retries++;
      delay(WIFI_RETRY_DELAY);
      Serial.print("#");
  }
  WiFi.setAutoReconnect(true);
  return WiFi.status(); // return the WiFi connection status
}
//---------------------------------------------------------------
void init_sensor() {
    if (!sensor.begin())
    Serial.println("Did not find Si7021 sensor!");

  sensor.heater(enableHeater);

  if (sensor.isHeaterEnabled())
    Serial.println("HEATER ENABLED");
  else
    Serial.println("HEATER DISABLED");
  
  Serial.print("Found model ");
  switch(sensor.getModel()) {
    case SI_Engineering_Samples:
      Serial.print("SI engineering samples"); break;
    case SI_7013:
      Serial.print("Si7013"); break;
    case SI_7020:
      Serial.print("Si7020"); break;
    case SI_7021:
      Serial.print("Si7021"); break;
    case SI_UNKNOWN:
    default:
      Serial.print("Unknown");
  }
  Serial.print(" Rev(");
  Serial.print(sensor.getRevision());
  Serial.print(")");
  Serial.print(" Serial #"); Serial.print(sensor.sernum_a, HEX); Serial.println(sensor.sernum_b, HEX);
}
//---------------------------------------------------------------
void getWeather() {
  // Measure Relative Humidity from the HTU21D or Si7021
  scout.set_value(HUM_IDX, sensor.readHumidity());

  // Measure Temperature from the HTU21D or Si7021
  scout.set_value(TEMP_IDX, sensor.readTemperature());
  // Temperature is measured every time RH is requested.
  // It is faster, therefore, to read it from previous RH
  // measurement with getTemp() instead with readTemp()
}

void getBatteryStatus(){
    for (int i = 0; i < FILTER_COUNT; i++){
      analogVals[i] = analogRead(A0);
      delay(1);
  }
  quicksort(analogVals, 0 , FILTER_COUNT - 1); // sort values
  float filtered_val = (analogVals[(int)floor(FILTER_COUNT/2)] + analogVals[((int)floor(FILTER_COUNT/2)) + 1]) / 2; // get mean of two middle values
  
  scout.set_value(VOLTAGE_IDX, ((filtered_val / 1023) * 4.2) + EMPIRICAL_VOLTAGE_OFFSET); // map 8 bit analog value to voltage, percent is also calculated

  Serial.print("Voltage: "); Serial.print(scout.get_value(VOLTAGE_IDX));
  Serial.print("\tPercent Charge: "); Serial.println(scout.get_value(PERCENT_IDX));
}

void httpHandleReturn(int httpCode){
        // httpCode will be negative on error
  if (httpCode > 0) {
    // HTTP header has been send and Server response header has been handled
    Serial.printf("[HTTP] POST... code: %d\n", httpCode);

    // file found at server
    if (httpCode == HTTP_CODE_OK) {
      const String& payload = http.getString();
      Serial.println("received payload:\n<<");
      Serial.println(payload);
      Serial.println(">>");
    }
  } else {
    Serial.printf("[HTTP] POST... failed, error: %s\n", http.errorToString(httpCode).c_str());
  }
}
