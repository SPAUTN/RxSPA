#include <Arduino.h>
#include <WiFiManager.h>
#include <Utils.h>

#define AT_CONTINUOUS_PRECV_CONFIG_SET "AT+PRECV=65534" // 65534 for continuous receive, 65535 for continuous receive until one reception.

bool initialized = false;
WiFiManager wifiManager;
String sendATCommand(String);
String readSerial2();
String hexToASCII(String);

void setup() {
  Serial.begin(115200);
  Serial2.begin(115200);
  wifiManager.autoConnect();
  String atCommandResetResponse = sendATCommand(Serial2, AT_RESET);
  String atConfigSetP2PResponse = sendATCommand(Serial2, AT_P2P_CONFIG_SET);
  String atConfigGetP2PResponse = sendATCommand(Serial2, AT_P2P_CONFIG_GET);
  String atConfigContRecvResponse = sendATCommand(Serial2, AT_CONTINUOUS_PRECV_CONFIG_SET);
}

void loop() {
      String rxData = readSerial(Serial2);
      rxData.trim();
      rxData = hexToASCII(rxData.substring(rxData.lastIndexOf(':')+1));
      Serial.print("Received: ");
      Serial.println(rxData);
}
