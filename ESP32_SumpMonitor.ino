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
  controlMotor(isSwitchOn(sensingPin),0,relayPin);
  delay(10);
}

boolean isSwitchOn(int pin) {
  return digitalRead(pin);
}
void controlMotor(bool switchState,bool hysteresisTimer,int pin) {
  if (switchState|hysteresisTimer) {
    digitalWrite(pin,1);
  }
  else {
    digitalWrite(pin,0);
  }
}
