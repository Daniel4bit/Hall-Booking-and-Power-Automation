#include <Arduino.h>

#include <Ethernet.h>
#include <MQTT.h>
#include <ArduinoJson.h>
#include <HttpUpdate.h>
#include <WiFi.h>

void update();

EthernetClient ethClient;
MQTTClient mqtt;
JsonDocument json;

const int warnDuration = 10;  //seconds before power off
float durationRemaining = 0;
unsigned long durationSetTime = 0;
int lastStatus = 0;
uint32_t updateCounter = 0;

const int outputPin = 26;
const int buzzerPin = 13;
const int warningLEDPin = 15;
const int wifiLEDPin = 32;

byte mac[] = { 0x74, 0x69, 0x69, 0x2D, 0x30, 0x32 };
IPAddress ip(10, 70, 11, 187);
IPAddress mydns(172, 16, 16, 16);
IPAddress gateway(10, 70, 11, 1);
IPAddress subnet(255, 255, 255, 0);

String deviceId = "D0005";
String roomId = "R0005";

const char* mqtt_server = "10.70.251.64";
String clientId = "clsrm-" + String(deviceId);
String subTopic = "qrpower/" + String(roomId) + "/" + String(deviceId);
String pubTopic = "qrpower/" + String(deviceId) + "/" + String(roomId);

WiFiClient wifiClient;
void update()
{
  String url = "http://cibikomberi.local:8080/thing/update/677367065ee782724fdba1a4?version=1";

  httpUpdate.update(wifiClient, url);
}

void parseDataFromMessage(String message) {
  DeserializationError error = deserializeJson(json, message);

  if (!error) {
    // if (json["device_id"].as<String>() != deviceId) {
    //   return;
    // }
    
    durationRemaining = json["minutes"].as<float>() * 60;
    durationSetTime = millis();

    if (durationRemaining > 0){
      JsonDocument publishDoc;
      publishDoc["device_id"] = deviceId;
      publishDoc["status"] = 1;
      mqtt.publish(pubTopic.c_str(), publishDoc.as<String>().c_str());
    } 
  }
}

//Method that is automatically called when there is a message
void MqttCallback(String &topic, String &payload) {
  Serial.print("Msg[");
  Serial.print(topic);
  Serial.print("]");
  Serial.println(payload);

  parseDataFromMessage(payload);
}

// Ensure connection with broker
void connectMqtt() {
  // No of tries to prevent infinite loop
  int i = 5;
  while (!mqtt.connected() && i > 0) {
    Serial.print("MQTT..");
    if (mqtt.connect(clientId.c_str())) {
      Serial.println("MQTT ok");
      mqtt.subscribe(subTopic);
      mqtt.setKeepAlive(10);
      mqtt.setWill("error", "device disconnected", true, 1);
      Serial.println(subTopic.c_str());
    } else {
      Serial.print("fail");
      // delay(500);
    }
    i--;
  }
}



void setup() {
  Serial.begin(9600);
  pinMode(outputPin, OUTPUT);
  pinMode(buzzerPin, OUTPUT);
  pinMode(warningLEDPin, OUTPUT);
  pinMode(wifiLEDPin, OUTPUT);
  digitalWrite(outputPin, LOW);
  digitalWrite(buzzerPin, LOW);
  digitalWrite(warningLEDPin, LOW);
  digitalWrite(wifiLEDPin, LOW);

  Ethernet.init(5);
  Ethernet.begin(mac, ip, mydns, gateway, subnet);
  
  Serial.print("IP:");
  Serial.println(Ethernet.localIP());
  digitalWrite(wifiLEDPin, HIGH);

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
  
  float elapsedTimeSinceLastMessage = (millis() - durationSetTime) / 1000;
  if (elapsedTimeSinceLastMessage < durationRemaining) {
    lastStatus = 1;
    digitalWrite(outputPin, HIGH);
    Serial.println("Time left: " + String(durationRemaining - elapsedTimeSinceLastMessage));
  } else {
    digitalWrite(outputPin, LOW);
    Serial.println("off");
  }

  // Warn before power off
  if ((durationRemaining - elapsedTimeSinceLastMessage) < warnDuration && (durationRemaining - elapsedTimeSinceLastMessage) > 0) {
    Serial.println("About to off");
    digitalWrite(buzzerPin, HIGH);
    digitalWrite(warningLEDPin, HIGH);
  } else {
    digitalWrite(buzzerPin, LOW);
    digitalWrite(warningLEDPin, LOW);
  }
  delay(500);

  //Send ack to server if power is transitioning to off
  if (lastStatus == 1 && durationRemaining - elapsedTimeSinceLastMessage <= 0) {
    JsonDocument publishDoc;
    publishDoc["device_id"] = deviceId;
    publishDoc["status"] = 0;

    mqtt.publish(pubTopic.c_str(), publishDoc.as<String>().c_str());
    lastStatus = 0;
    // ESP.restart();
  }  


}