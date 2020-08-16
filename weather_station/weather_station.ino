#include <Adafruit_Si7021.h>
#include <stdio.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>  // Requires version 5
#include "config.h" // Contains WLAN_SSID and WLAN_PASS defines
#include <Wire.h>

// API parameters
#define HTTP_REST_PORT 80
#define WIFI_RETRY_DELAY 500
#define MAX_WIFI_INIT_RETRY 50



float humidity = 0;
float temp = 0;
bool enableHeater = false;
// Create instance of si7021 object from adafruit library
Adafruit_Si7021 sensor = Adafruit_Si7021();
// Create instance of server object
ESP8266WebServer http_rest_server(HTTP_REST_PORT);

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
  config_rest_server_routing();

  http_rest_server.begin();
  Serial.println("HTTP REST Server Started");
}

void loop() {
  http_rest_server.handleClient();
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
