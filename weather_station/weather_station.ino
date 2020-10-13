#include <Adafruit_Si7021.h>
#include <ESP8266WiFi.h>
#include <stdio.h>
#include <math.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>  // Requires version 5
#include "config.h" // Contains WLAN_SSID and WLAN_PASS defines
#include "utils.c"
#include <Wire.h>

// API parameters
#define HTTP_REST_PORT 80
#define WIFI_RETRY_DELAY 500
#define MAX_WIFI_INIT_RETRY 50


#define FILTER_COUNT 50 // amount of samples to take for battery voltage filtering
#define EMPIRICAL_VOLTAGE_OFFSET 0.07
#define WARN_BATTERY_VOLTAGE 3.3
#define CRITICAL_BATTERY_VOLTAGE 3
#define WARNING_LED 14
#define CRITICAL_LED 12

#define BATTERY_HEALTH_CHECK_INTERVAL_MIN 10

// Weather information
float humidity = 0;
float temp = 0;

// Battery information
float analogVals[FILTER_COUNT] = {0};
float voltage = 0;
float percent_charge = 0;

unsigned long last_query = 0;

bool enableHeater = false;
// Create instance of si7021 object from adafruit library
Adafruit_Si7021 sensor = Adafruit_Si7021();
// Create instance of server object
ESP8266WebServer http_rest_server(HTTP_REST_PORT);

void setup() {
  Serial.begin(115200);
  Serial.println();

  pinMode(WARNING_LED, OUTPUT);
  pinMode(CRITICAL_LED, OUTPUT);

  WiFi.setSleepMode(WIFI_LIGHT_SLEEP, 3);  // Automatic Light Sleep, DTIM listen interval = 3

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
  config_rest_server_routing();

  http_rest_server.begin();
  Serial.println("HTTP REST Server Started");
}

void loop() {
  http_rest_server.handleClient();

  // Run a health check if we haven't gotten any queries in a while
  if (millis() - last_query >= (BATTERY_HEALTH_CHECK_INTERVAL_MIN * 60 * 1000)){
    battery_healthcheck();
    last_query = millis();   
  }
}

void http_get_weather() {
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& jsonObj = jsonBuffer.createObject();
  char JSONmessageBuffer[200];

  getWeather();
  jsonObj["humidity"] = humidity;
  jsonObj["temperature"] = temp;
  jsonObj.prettyPrintTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));
  http_rest_server.send(200, "application/json", JSONmessageBuffer);
}

void http_get_battery_status() {
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& jsonObj = jsonBuffer.createObject();
  char JSONmessageBuffer[200];

  // Also queries and updates the voltage and percentage
  battery_healthcheck();

  jsonObj["voltage"] = voltage;
  jsonObj["percent_charge"] = percent_charge;
  jsonObj.prettyPrintTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));
  http_rest_server.send(200, "application/json", JSONmessageBuffer);
  last_query = millis();
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
void config_rest_server_routing() {
    http_rest_server.on("/", HTTP_GET, []() {
        http_rest_server.send(200, "text/html",
            "Welcome to the ESP8266 REST Web Server");
    });
    http_rest_server.on("/weather", HTTP_GET, http_get_weather);
    http_rest_server.on("/battery", HTTP_GET, http_get_battery_status);
}
//---------------------------------------------------------------
void getWeather() {
  // Measure Relative Humidity from the HTU21D or Si7021
  humidity = sensor.readHumidity();

  // Measure Temperature from the HTU21D or Si7021
  temp = sensor.readTemperature();
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
  
  voltage = ((filtered_val / 1023) * 4.2) + EMPIRICAL_VOLTAGE_OFFSET; // map 8 bit analog value to voltage
  percent_charge = ((voltage - 3) / 1.2) * 100; // map voltage to percentage of charge

  Serial.print("Voltage: "); Serial.print(voltage);
  Serial.print("\tPercent Charge: "); Serial.println(percent_charge);
}

void battery_healthcheck(){
    getBatteryStatus();
    if (voltage <= WARN_BATTERY_VOLTAGE)
      digitalWrite(WARNING_LED, HIGH);
    else if (voltage <= CRITICAL_BATTERY_VOLTAGE){
      digitalWrite(WARNING_LED, LOW);
      digitalWrite(CRITICAL_LED, HIGH);
    }
    else {
      digitalWrite(WARNING_LED, LOW);
      digitalWrite(CRITICAL_LED, LOW);
    }
}


