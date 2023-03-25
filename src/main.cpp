#include <Arduino.h>
#include <WiFiManager.h>

#define AT_RESET "ATZ"
#define AT_P2P_CONFIG_SET "AT+P2P=915000000:7:0:0:10:14"
#define AT_P2P_CONFIG_GET "AT+P2P=?"
#define AT_CONTINUOUS_PRECV_CONFIG_SET "AT+PRECV=65534" // 65534 for continuous receive, 65535 for continuous receive until one reception.
#define AT_CONTINUOUS_PRECV_CONFIG_GET "AT+PRECV=?"

bool initialized = false;
WiFiManager wifiManager;
String sendATCommand(String);

void setup() {
  Serial.begin(115200);
  Serial2.begin(115200);
  wifiManager.autoConnect();
}

void loop() {
  if(!initialized) {
    String resp0 = sendATCommand(AT_RESET);
    String resp1 = sendATCommand(AT_P2P_CONFIG_SET);
    String resp2 = sendATCommand(AT_P2P_CONFIG_GET);
    String resp3 = sendATCommand(AT_CONTINUOUS_PRECV_CONFIG_SET);
    initialized = true;
    Serial1.flush();
    delay(1000);
    return;
  } else {
    // Process received data
  }
}

String sendATCommand(String command) {
  String response = "";
  bool configCommand = command.indexOf('?') == -1;
  if(configCommand) {
    delay(1000);
  }
  Serial2.flush();
  Serial2.println();
  Serial2.println(command);
  delay(500);
  Serial2.flush();
  while(Serial2.available()>0) {
    char c = Serial2.read();
    Serial.print(c);
    response += c;
  }
  if( !configCommand ) {
    response = response.substring(0, response.indexOf('\r'));
  }
  response.trim();
  return response;
}