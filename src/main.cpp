#include <Arduino.h>

#include <Ethernet.h>
#include <MQTT.h>
#include <ArduinoJson.h>
#include <HttpUpdate.h>
#include <WiFi.h>

void update();

EthernetClient ethClient;
WiFiClient wifiClient;
MQTTClient mqtt;
JsonDocument json;

const int warnDuration = 10;  //seconds before power off

uint32_t updateCounter = 0;


String deviceId = "D0005";
String rooms[] = {"R0005", "R0006", "R0007"};
const int relay[] = {26,27,28};
const int n = sizeof(relay) / sizeof(relay[0]);

float durationRemaining[n];
unsigned long durationSetTime[n];
int lastStatus[n];

const int buzzerPin = 13;
const int warningLEDPin = 15;
const int networkStatusLed = 32;

byte mac[] = { 0x74, 0x69, 0x69, 0x2D, 0x30, 0x32 };
IPAddress ip(10, 70, 11, 187);
IPAddress mydns(172, 16, 16, 16);
IPAddress gateway(10, 70, 11, 1);
IPAddress subnet(255, 255, 255, 0);

const char* mqtt_server = "10.70.11.247";
String clientId = "clsrm-" + String(deviceId);

void update() {
  String url = "http://cibikomberi.local:8080/thing/update/677367065ee782724fdba1a4?version=1";
  httpUpdate.update(wifiClient, url);
}

void parseData(String topic, String message) {
  String roomId = topic.substring(topic.indexOf("/"), topic.lastIndexOf("/"));
  DeserializationError error = deserializeJson(json, message);

  if (!error) {
    // if (json["device_id"].as<String>() != deviceId) {
    //   return;
    // }
    for (int i = 0; i < n; i++) {
      if (rooms[i].equals(roomId)) {
        durationRemaining[i] = json["minutes"].as<float>() * 60;
        durationSetTime[i] = millis();

        if (durationRemaining[i] > 0){
          JsonDocument publishDoc;
          publishDoc["device_id"] = deviceId;
          publishDoc["status"] = 1;
          mqtt.publish(("qrpower/" + deviceId + "/" + roomId), publishDoc.as<String>().c_str());
        } 
      }
    }
  }
}

//Method that is automatically called when there is a message
void MqttCallback(String &topic, String &payload) {
  Serial.print("Msg[");
  Serial.print(topic);
  Serial.print("]");
  Serial.println(payload);

  parseData(topic, payload);
}

// Ensure connection with broker
void connectMqtt() {
  // No of tries to prevent infinite loop
  int i = 5;
  digitalWrite(networkStatusLed, HIGH);
  while (!mqtt.connected() && i > 0) {
    Serial.print("MQTT..");

    // Will must be set before connecting 
    mqtt.setWill("error", "device disconnected");
    if (mqtt.connect(clientId.c_str())) {
      Serial.println("MQTT ok");
      digitalWrite(networkStatusLed, LOW);

      for (int i = 0; i < n; i++) {
        mqtt.subscribe("qrpower/" + String(rooms[i]) + "/" + String(deviceId));
      }
    } else {
      Serial.println("fail");
      // delay(500);
    }
    i--;
  }
}


void setup() {
  Serial.begin(9600);

  pinMode(buzzerPin, OUTPUT);
  pinMode(warningLEDPin, OUTPUT);
  pinMode(networkStatusLed, OUTPUT);

  digitalWrite(buzzerPin, LOW);
  digitalWrite(warningLEDPin, LOW);
  digitalWrite(networkStatusLed, LOW);

  for (int i = 0; i < n; i++) {
    pinMode(relay[i], OUTPUT);
    digitalWrite(relay[i], LOW);
  }

  Ethernet.init(5);
  Ethernet.begin(mac, ip, mydns, gateway, subnet);

  Serial.print("IP:");
  Serial.println(Ethernet.localIP());
  
  mqtt.begin(mqtt_server, 1883, ethClient);
  mqtt.onMessage(MqttCallback);
  connectMqtt();

  WiFi.begin("BIT-ENERGY", "pic-embedded");
}

void loop() {
  if (!mqtt.connected()) {
    connectMqtt();
  }
  mqtt.loop();
  if (WiFi.status() == WL_CONNECTED) {
    update();
  }
  

  for (int i = 0; i < n; i++) {
    float elapsedTime = (millis() - durationSetTime[i]) / 1000;
    if (elapsedTime < durationRemaining[i]) {
      lastStatus[i] = 1;
      digitalWrite(relay[i], HIGH);
      Serial.println("Device" + String(i) +  ": " + String(durationRemaining[i] - elapsedTime));
    } else {
      digitalWrite(relay[i], LOW);
    }
    
    if ((durationRemaining[i] - elapsedTime) < warnDuration && (durationRemaining[i] - elapsedTime) > 0) {
      digitalWrite(buzzerPin, HIGH);
      digitalWrite(warningLEDPin, HIGH);
    } else {
      digitalWrite(buzzerPin, LOW);
      digitalWrite(warningLEDPin, LOW);
    }

    if (lastStatus[i] == 1 && durationRemaining[i] - elapsedTime <= 0) {
      JsonDocument publishDoc;
      publishDoc["device_id"] = deviceId;
      publishDoc["status"] = 0;

      mqtt.publish("qrpower/" + deviceId + "/" + rooms[i], publishDoc.as<String>());
      lastStatus[i] = 0;
      // ESP.restart();
    } 
  }
}