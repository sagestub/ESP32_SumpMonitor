#include "HardwareSerial.h"
#include "esp_adc/adc_continuous.h" //continuous mode adc provides faster read


const int relayPin = 3;
const int sensingPin = 17;
const int pwmPin = 25;            //GPIO pin for pwm output
const int pwmDutyCycle = 39;     //PWM duty cycle (0-255)
const int adcPin = 36;
const int baudRate=250000;
boolean relayState = 0;
int val = 0;
int maxVal = 0;
unsigned int intBuffer = 0;
float avg = 0;

unsigned int count = 0;
void setup() {
  Serial.begin(baudRate);
  pinMode(relayPin,OUTPUT);
  pinMode(sensingPin,INPUT_PULLUP);
  analogSetAttenuation(ADC_11db);
  pinMode(adcPin,INPUT);
//  pinMode(pwmPin,OUTPUT);
//  analogWrite(pwmPin,pwmDutyCycle);
}

void loop() {
  controlMotor(isSwitchOn(sensingPin),0,relayPin);
  val=analogRead(adcPin);
  if(val>maxVal) {
    maxVal = val;
  }
  count++; 
  if(count>50){
    avg = float(maxVal)*3.3/4096*1.18*32; //0.707*3.3/4096*32, 0.018
    intBuffer=0;
    count = 0;
    maxVal = 0;
    Serial.println(avg);
  }
  //Serial.println(count);
  delay(5);
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
