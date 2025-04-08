#include "HardwareSerial.h"
int relayPin = 3;
int sensingPin = 18;
int baudRate=115200;
boolean relayState = 0;


void setup() {
  Serial.begin(baudRate);
  pinMode(relayPin,OUTPUT);
  pinMode(sensingPin,INPUT_PULLUP);

}

void loop() {
  digitalWrite(relayPin,0);
  relayState = digitalRead(sensingPin);
  Serial.print("Relay is ");
  Serial.println(relayState);
  delay(1000);
  digitalWrite(relayPin,1);
  relayState = digitalRead(sensingPin);
  Serial.print("Relay is ");
  Serial.println(relayState);
  delay(1000);
  

}
