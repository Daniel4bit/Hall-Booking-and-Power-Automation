#include <Arduino.h>

#include <Ethernet.h>
#include <PubSubClient.h>

byte mac[] = { 0x74, 0x69, 0x69, 0x2D, 0x30, 0x31 };
IPAddress ip(10, 70, 11, 187);
IPAddress mydns(172, 16, 16, 16);
IPAddress gateway(10, 70, 11, 1);
IPAddress subnet(255, 255, 255, 0);

String deviceId = "D001";
String roomId = "R001";

const char* mqtt_server = "test.mosquitto.org";
String clientId = "clsrm-node-" + String(deviceId);
String subTopic = "qrpower/" + roomId + "/" + deviceId;
String pubTopic = "qrpower/" + deviceId + "/" + roomID;

EthernetClient ethClient;
PubSubClient MqttClient(ethClient);

const int warnDuration = 10;  //seconds before power off
const int outputPin = 26;
const int buzzerPin = 13;
const int warningLEDPin = 15;
const int wifiLEDPin = 32;
float durationRemaining = 0;
unsigned long durationSetTime = 0;
int bookingID = 0;
int roomID = 0;

void parseDataFromMessage(String message) {
  int firstComma = message.indexOf(',');
  int lastComma = message.lastIndexOf(',');

  if (firstComma != -1 && lastComma != -1 && firstComma != lastComma) {
    bookingID = message.substring(0, firstComma).toInt();
    roomID = message.substring(firstComma + 1, lastComma).toInt();

    //convert seconds to float in milli seconds
    durationRemaining = (message.substring(lastComma + 1).toFloat());
    durationSetTime = millis();

    Serial.println("ID: " + String(bookingID));
    Serial.println("RoomID: " + String(roomID));
    Serial.println("time left: " + String(durationRemaining) + "s");

  } else {
    Serial.println("Parse fail");
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
  Serial.println();

  // if (String(topicIn).equals(subTopic)) {
  parseDataFromMessage(message);
  // }
}

void sendStatusUpdate() {
  // if (WiFi.status() == WL_CONNECTED) {
  String statusString = String(bookingID) + "," + String(roomID) + "," + String(durationRemaining - ((millis() - durationSetTime) / 1000));
  MqttClient.publish(pubTopic.c_str(), statusString.c_str());
  // } else {
  //   connectWifi();
  // }
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
  sendStatusUpdate();

  // current time - durationSetTime gives the time after last message
  float elapsedTimeSinceLastMessage = (millis() - durationSetTime) / 1000;
  // if last message was 10 sec ago and if duration remaining is 5 seconds it will turn off
  if (elapsedTimeSinceLastMessage < durationRemaining) {
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
}
