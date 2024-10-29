#include <Servo.h>

Servo myServo;   // Create a servo object
int potPin = A0; // Pin where the potentiometer is connected
int potValue = 0; // Value read from the potentiometer
int angle = 0;    // Servo angle (0 - 180)
bool isRunning = true; // Flag to control program execution

void setup() {
  Serial.begin(9600); // Start serial communication
  myServo.attach(9);  // Attach the servo to pin 9
  Serial.println("Servo Control Started");
  Serial.println("Enter 's' to stop the program");
}

void loop() {
  if (Serial.available() > 0) {
    char input = Serial.read();
    if (input == 's' || input == 'S') {  // Accept both lower and uppercase 's'
      isRunning = false;
      myServo.write(90);  // Move servo to neutral position when stopping
      Serial.println("Program stopped. Reset Arduino to restart.");
      while(true) {  // Hold program in infinite loop
        delay(1000);
      }
    }
  }

  if (isRunning) {
    potValue = analogRead(potPin);            // Read the potentiometer value (0-1023)
    angle = map(potValue, 0, 1023, 0, 180);   // Map the pot value to a range for the servo (0-180)
    myServo.write(angle);                     // Set the servo angle
    
    // Print formatted output
    Serial.print("Servo Angle: ");
    Serial.println(angle);
    
    delay(100);  // Small delay for stability
  }
}