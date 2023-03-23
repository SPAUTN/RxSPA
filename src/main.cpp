#include <Arduino.h>

void setup() {
  Serial.begin(9600);
  Serial1.begin(9600);
}

void loop() {
  if(Serial.available()) {
    char c = Serial.read();
    Serial1.println(c);
  }
  if(Serial1.available()) {
    char c = Serial1.read();
    Serial.println(c);
  }
}