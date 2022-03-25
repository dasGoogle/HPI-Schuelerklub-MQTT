#include "MQTT.h"
#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>

MQTTClass::MQTTClass(void (*internal_callback)(char* topicBytes, byte* messageBuffer, unsigned int length)) {
  this->wiFiClient = WiFiClient();
  this->client = PubSubClient(this->wiFiClient);

  this->topics = TopicList();

  this->internal_callback = internal_callback;
}

MQTTClass::~MQTTClass() {
  free(this->server);
}

void MQTTClass::setup(String ssid, String password, String server, int port, void (*callback_ptr)(String topic, String message)) {
  // Copy server Domain/IP
  // PubSub does not copy the string itself, so it's missing on a possibly necessary reconnect
  const unsigned int length = server.length() + 1;
  this->server = (char*)calloc(length, sizeof(char));
  server.toCharArray(this->server, length);

  this->external_callback = callback_ptr;

  delay(10);
  Serial.println("Connecting to " + ssid);

  WiFi.begin(ssid.c_str(), password.c_str());

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("========== WiFi connected ==========");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  this->client.setServer(this->server, port);
  this->client.setCallback(this->internal_callback);

  this->connect();
}

void MQTTClass::connect() {
  while (!this->client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (this->client.connect("ESP8266Client")) {
      Serial.println("connected");
      this->topics.subscribe_all(&this->client);
    } else {
      Serial.print("failed, rc=");
      Serial.print(this->client.state());
      Serial.println(" try again in 5 seconds");

      delay(5000);
    }
  }
}

void MQTTClass::loop() {
  if (!this->client.connected()) {
    this->connect();
  }

  this->client.loop();
}

external_callback_t MQTTClass::get_external_callback() {
  return this->external_callback;
}

void MQTTClass::subscribe(String topic) {
  this->client.subscribe(topic.c_str());
  this->topics.add(topic);
}

void MQTTClass::publish(String topic, String message) {
  this->client.publish(topic.c_str(), message.c_str());
}

void MQTTClass::publish(String topic, int message) {
  this->publish(topic, String(message));
}

void MQTTClass::publish(String topic, float message){
  this->publish(topic, String(message));
}

MQTTClass MQTT (&mqtt_callback);

void mqtt_callback(char* topicBytes, byte* messageBuffer, unsigned int length) {
  String topic = topicBytes;
  String message;

  for (int i = 0; i < length; i++) {
    message += (char)messageBuffer[i];
  }

  MQTT.get_external_callback()(topic, message);
}