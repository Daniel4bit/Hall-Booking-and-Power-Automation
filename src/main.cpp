#include <Arduino.h>

#include <Ethernet.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

EthernetClient ethClient;
PubSubClient MqttClient(ethClient);
JsonDocument json;

const int warnDuration = 10;  //seconds before power off
float durationRemaining = 0;
unsigned long durationSetTime = 0;
int lastStatus = 0;

const int outputPin = 26;
const int buzzerPin = 13;
const int warningLEDPin = 15;
const int wifiLEDPin = 32;

byte mac[] = { 0x74, 0x69, 0x69, 0x2D, 0x30, 0x31 };
IPAddress ip(10, 70, 11, 187);
IPAddress mydns(172, 16, 16, 16);
IPAddress gateway(10, 70, 11, 1);
IPAddress subnet(255, 255, 255, 0);

String deviceId = "D0001";
String roomId = "R0001";

const char* mqtt_server = "10.70.11.247";
String clientId = "clsrm-node-" + String(deviceId);
String subTopic = "qrpower/" + String(roomId) + "/" + String(deviceId);
String pubTopic = "qrpower/" + String(deviceId) + "/" + String(roomId);




void parseDataFromMessage(String message) {
  DeserializationError error = deserializeJson(json, message);

  if (!error) {
    if (json["device_id"].as<String>() != deviceId) {
      return;
    }
    
    durationRemaining = json["minutes"].as<float>() * 60;
    durationSetTime = millis();

    if (durationRemaining > 0){
      JsonDocument publishDoc;
      publishDoc["device_id"] = deviceId;
      publishDoc["status"] = 1;
      MqttClient.publish(pubTopic.c_str(), publishDoc.as<String>().c_str());
    } 
  }
}

//Method that is automatically called when there is a message
void MqttCallback(char* topicIn, byte* messageIn, unsigned int length) {
  Serial.print("Msg[");
  Serial.print(topicIn);
  Serial.print("]");
  String message;

  for (int i = 0; i < length; i++) {
    Serial.print((char)messageIn[i]);
    message += (char)messageIn[i];
  }

  parseDataFromMessage(message);
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

  MqttClient.setServer(mqtt_server, 1883);
  MqttClient.setCallback(MqttCallback);
  MqttClient.subscribe(subTopic.c_str());
}

// Ensure connection with broker
void reconnectMqtt() {
  // No of tries to prevent infinite loop
  int i = 5;
  while (!MqttClient.connected() && i > 0) {
    Serial.print("MQTT..");
    if (MqttClient.connect(clientId.c_str())) {
      Serial.println("MQTT ok");
      MqttClient.subscribe(subTopic.c_str());
      Serial.println(subTopic.c_str());
    } else {
      Serial.print("fail, st=");
      Serial.print(MqttClient.state());
      Serial.println("try again");
      delay(500);
    }
    i--;
  }
}

void loop() {
  if (!MqttClient.connected()) {
    reconnectMqtt();
  }
  MqttClient.loop();

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

    MqttClient.publish(pubTopic.c_str(), publishDoc.as<String>().c_str());
    lastStatus = 0;
  }  
}
