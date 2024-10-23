#include <Servo.h>

Servo servo;

int angle = 90;

void setup() 
{
  servo.attach(9); // Attach the servo to digital pin 9
}

void loop() 
{
  servo.write(angle); // Set to the desired angle
  delay(1000); // Wait for 1 second
  // Reverse the angle between 90 and 180 degrees
  if (angle == 90) {
    angle = 180;
  } else {
    angle = 90;
  }
}
