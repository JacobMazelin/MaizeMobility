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


//Extra Added Code
// Move down (angle numbers might need to be altered if incorrect)
  for (int angle = 135; angle >= 0; angle--) {
    MG90S.write(angle);
    delay(15);
  }

  delay(10000); // Pause for 10 seconds for the passenger to get on

  // Move back up
  for (int angle = 0; angle <= 135; angle++) {
    MG90S.write(angle);
    delay(15);
  }

  delay(1000); // Pause again
}
