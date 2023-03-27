#include <Arduino.h>
#include <WiFiManager.h>
#include <SoftwareSerial.h>

#define portSpeed 38400
#define AT_RESET "ATZ"
#define AT_BAUD_CONFIG_SET "AT+BAUD=38400"
#define AT_P2P_CONFIG_SET "AT+P2P=915000000:7:0:0:10:14"
#define AT_P2P_CONFIG_GET "AT+P2P=?"
#define AT_CONTINUOUS_PRECV_CONFIG_SET "AT+PRECV=65534" // 65534 for continuous receive, 65535 for continuous receive until one reception.

SoftwareSerial Serial2;
WiFiManager wifiManager;
bool initialized = false;
String sendATCommand(String);
String readSerial2();
String hexToASCII(String);

void setup() {
  Serial.begin(portSpeed);
  Serial2.begin(portSpeed, SWSERIAL_8N1, 12, 13, false);
  while(!Serial2) {
    Serial.println("Serial2 initialization failed!");
    delay(1000);
  }

  wifiManager.autoConnect();
}

void loop() {
  if(!initialized) {
    // Responses stored in variables for debug and/or future validations
    String atCommandResetResponse = sendATCommand(AT_RESET);
    String atConfigSetBaudReponse = sendATCommand(AT_BAUD_CONFIG_SET);
    String atConfigSetP2PResponse = sendATCommand(AT_P2P_CONFIG_SET);
    String atConfigGetP2PResponse = sendATCommand(AT_P2P_CONFIG_GET);
    String atConfigContRecvResponse = sendATCommand(AT_CONTINUOUS_PRECV_CONFIG_SET);
    initialized = true;
    Serial1.flush();
    delay(1000);
    return;
  } else {
    // Process received data
    if(Serial2.available()>0) {
      String rxData = readSerial2();
      rxData.trim();
      rxData = hexToASCII(rxData.substring(rxData.lastIndexOf(':')+1));
      Serial.print("Received: ");
      Serial.println(rxData);
    }
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
  response = readSerial2();
  if( !configCommand ) {
    response = response.substring(0, response.indexOf('\r'));
  }
  response.trim();
  return response;
}

String readSerial2() {
  String readed = "";
  while(Serial2.available()>0) {
    char c = Serial2.read();
    Serial.print(c);
    readed += c;
    delay(10);
  }
  return readed;
}

String hexToASCII(String hex) {
  String ascii = "";
  for(int i=0; i<hex.length(); i+=2) {
    String ch = hex.substring(i, i+2);
    char c = (char) (int)strtol(ch.c_str(), NULL, HEX);
    ascii += (char) c;
  }
  return ascii;
}