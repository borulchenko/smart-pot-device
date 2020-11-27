// This example uses an Adafruit Huzzah ESP8266
// to connect to shiftr.io.
//
// You can check on your device after a successful
// connection here: https://shiftr.io/try.
//
// by Joël Gähwiler
// https://github.com/256dpi/arduino-mqtt

#include <ESP8266WiFi.h>
#include <MQTT.h>
#include <dht11.h>
#include <ArduinoJson.h>

dht11 DHT;
#define DHT11_PIN 16
#define SOIL_HUMIDITY_PIN 0
#define ILLUMINATION_PIN 13

const char ssid[] =  "Кислород";
const char pass[] =  "Czkxd0308bnexzK";

WiFiClient net;
MQTTClient client;

unsigned long lastMillis = 0;
boolean collectDataReceived = false;
boolean enableWateringReceived = false;
boolean enableLighting = false;
boolean enableLightingReceived = false;

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, pass);

  // Note: Local domain names (e.g. "Computer.local" on OSX) are not supported by Arduino.
  // You need to set the IP address directly.
  client.begin("broker.shiftr.io", net);
  client.onMessage(processMessage);
  connect();

  Serial.print("DHT LIBRARY VERSION: ");
  Serial.println(DHT11LIB_VERSION);

  pinMode(ILLUMINATION_PIN, OUTPUT);         //Set Pin13 as light output
  digitalWrite(ILLUMINATION_PIN, HIGH);
}

void processMessage(String &topic, String &payload) {
  if (topic == "/COLLECT_STATUS") {
    collectDataReceived = true;
  } else if (topic == "/ENABLE_WATERING") {
    enableWateringReceived = true;
  } else if (topic == "/ENABLE_LIGHTING") {
    enableLighting = payload == "true";
    enableLightingReceived = true;
  } else {
    Serial.print("Unknown topic was recieved:");
  }
  Serial.println("Incoming Topic: " + topic + ", payload: " + payload);
}


void connect() {
  Serial.print("checking wifi...");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
  }

  Serial.print("\nconnecting...");
  while (!client.connect("smartpot-device", "79b5b7ac", "7614b0c8da666f35")) {
    Serial.print(".");
    delay(1000);
  }
  Serial.println("\nconnected!");
  client.subscribe("/COLLECT_STATUS");
  client.subscribe("/ENABLE_LIGHTING");
}

void loop() {
  client.loop();
  delay(500);  // <- fixes some issues with WiFi stability

  if (!client.connected()) {
    connect();
  }

  if (collectDataReceived) {
    collectAndSendData();
  }
  if (enableLightingReceived) {
    enableLightingRelay();
    enableLightingReceived = false;
  }
}


void collectAndSendData() {
  int chk;
  Serial.print("Type: DHT11,\t Status: ");
  chk = DHT.read(DHT11_PIN);    // READ DATA
  switch (chk) {
    case DHTLIB_OK:
      Serial.print("OK,\t");
      break;
    case DHTLIB_ERROR_CHECKSUM:
      Serial.print("Checksum error,\t");
      break;
    case DHTLIB_ERROR_TIMEOUT:
      Serial.print("Time out error,\t");
      break;
    default:
      Serial.print("Unknown error,\t");
      break;
  }

  int humidity = DHT.humidity;
  int temperature = DHT.temperature;
  
  Serial.println("humidity : " + humidity);
  Serial.println("temperature : " + temperature);
  delay(50);
  int soilMoisture = analogRead(SOIL_HUMIDITY_PIN) / 10;

  //  const int capacity = JSON_OBJECT_SIZE(4);
  StaticJsonDocument<3> payloadDoc;
  payloadDoc["humidity"] = humidity;
  payloadDoc["temperature"] = temperature;
  payloadDoc["soilMoisture"] = soilMoisture;

  String payload = "";
  serializeJsonPretty(payloadDoc, payload);

  Serial.println("Payload: " + payload);
  collectDataReceived = false;

  delay(500);  // <- fixes some issues with WiFi stability
  client.publish("/STORE_STATUS", payload);
}

void enableLightingRelay() {
  if (enableLighting) {
    
    Serial.println("enableLighting: set to high" );
    digitalWrite(ILLUMINATION_PIN, HIGH);    //Turn on lights
  } else {
    
    Serial.println("enableLighting: set to low" );
    digitalWrite(ILLUMINATION_PIN, LOW);    //Turn off lights
  }
}



//mosquitto_sub -h broker.shiftr.io -p 1883 -u 79b5b7ac -P 7614b0c8da666f35  -t test
//mosquitto_pub -h broker.shiftr.io -p 1883 -u 79b5b7ac -P 7614b0c8da666f35  -t test -m messagge
