#include <Arduino.h>
#include <WiFiManager.h>

#define AT_P2P_CONFIG_SET "AT+P2P=915000000:7:0:0:10:14"
#define AT_P2P_CONFIG_GET "AT+P2P=?"
#define AT_CONTINUOUS_PRECV_CONFIG_SET "AT+PRECV=65534"
#define AT_CONTINUOUS_PRECV_CONFIG_GET "AT+PRECV=?"

bool initialized = false;
String response = "";

WiFiManager wifiManager;
void sendATCommand(String);

void setup() {
  Serial.begin(115200);
  Serial2.begin(115200);
  wifiManager.autoConnect();
}

void loop() {
  if(!initialized) {
    delay(500);
    Serial2.println();
    sendATCommand(AT_P2P_CONFIG_SET);
    sendATCommand(AT_P2P_CONFIG_GET);
    sendATCommand(AT_CONTINUOUS_PRECV_CONFIG_SET);
    initialized = true;
    response = "";
    Serial1.flush();
    return;
  }
  if(Serial2.available()>0) {
    char c = Serial2.read();
    Serial.print(c);
    response += c;
    return;
  }
  if(response.length() > 0) {
    if(response.indexOf("OK") > -1) {
      Serial.print("RESPONSE: ");
      Serial.println(response);
      Serial.println("-----------------");
    } else {
      Serial.println("RESPONSE: ERROR");
    }
    response = "";
  }
}

void sendATCommand(String command) {
  Serial2.flush();
  Serial2.println();
  Serial2.println(command);
  delay(500);
  Serial2.flush();
}