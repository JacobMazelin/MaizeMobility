#include <Servo.h>

Servo MG90S;

int potentiometerPin = 0;
int pulsePin = 9;
int analogVal = 0;
int convertVal = 0;

void setup()
{

MG90S.attach(pulsePin);

  
}

void loop()
{

 analogVal = analogRead(potentiometerPin);
 convertVal = map(analogVal, 0, 1023, 0, 180); 
 MG90S.write(convertVal);
 delay(15);
  
}