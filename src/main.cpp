#include <Arduino.h>

void setup() {
  Serial.begin(9600);
  Serial1.begin(9600);
}

void loop() {
  if(Serial.available()) {
    String s = Serial.readString();
    Serial.print("String received: ");
    Serial.println(s);
  }
}