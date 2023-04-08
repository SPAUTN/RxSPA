#include <Arduino.h>
#include <WiFiManager.h>
#include <PubSubClient.h>
#include <Utils.h>

#define AT_CONTINUOUS_PRECV_CONFIG_SET "AT+PRECV=65534" // 65534 for continuous receive, 65535 for continuous receive until one reception.

const char* MQTT_SERVER = "au1.cloud.thethings.network";
const int MQTT_PORT = 1883;
const char* MQTT_USER = "spav1@ttn";
const char* MQTT_PASSWORD = "NNSXS.WHJTHYJ7OSO6NNOERMYNFEWR63NXGSBK7XDHACQ.4S3O54HT2DWOJXWZDMEWDMB4K4Z3OUXXUZUT73SHDMZMJ6MWUK5Q";
const char* MQTT_TOPIC = "test/topic";

bool initialized = false;
WiFiManager wifiManager;
PubSubClient mqttClient;

void callback(char*, byte*, unsigned int);

void setup() {
  Serial.begin(115200);
  Serial2.begin(115200);
  wifiManager.autoConnect();

  mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
  mqttClient.setCallback(callback);

  while (!mqttClient.connected()) {
    Serial.println("Connecting to MQTT...");
    if (mqttClient.connect("ESP32", MQTT_USER, MQTT_PASSWORD)) {
      Serial.println("Connected to MQTT");
      mqttClient.subscribe(MQTT_TOPIC);
    } else {
      Serial.print("Failed to connect to MQTT, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }

  // String atCommandResetResponse = sendATCommand(Serial2, AT_RESET);
  // String atConfigSetP2PResponse = sendATCommand(Serial2, AT_P2P_CONFIG_SET);
  // String atConfigGetP2PResponse = sendATCommand(Serial2, AT_P2P_CONFIG_GET);
  // String atConfigContRecvResponse = sendATCommand(Serial2, AT_CONTINUOUS_PRECV_CONFIG_SET);
}

void loop() {
  String rxData = readSerial(Serial2);
  rxData.trim();
  rxData = hexToASCII(rxData.substring(rxData.lastIndexOf(':')+1));
  Serial.print("Received: ");
  Serial.println(rxData);
  mqttClient.publish(MQTT_TOPIC, rxData.c_str());
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}