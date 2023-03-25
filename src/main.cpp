#include <Arduino.h>

#define AT_P2P_CONFIG "AT+P2P=915000000:7:0:0:10:14"
bool initialized = false;

String response = "";

void setup() {
  Serial.begin(115200);
  Serial2.begin(115200);
}

void loop() {

  if(!initialized) {
    delay(500);
    Serial2.println(AT_P2P_CONFIG);
    initialized = true;
    return;
  }

  if(Serial2.available()>0) {
    char c = Serial2.read();
    Serial.print(c);
    response += c;
    return;
  }
  if(response.length() > 0) {
    Serial.print("RESPONSE: ");
    Serial.println(response);
    if(response.indexOf("OK") > -1) {
      Serial.println("OK");
    }
    response = "";
  }
}