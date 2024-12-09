#include <SoftwareSerial.h>

// Define RX and TX pins for SoftwareSerial
SoftwareSerial BTSerial(2, 3); // RX | TX

void setup() {
  // Initialize the hardware serial port (USB)
  Serial.begin(9600);
  // Initialize the Bluetooth serial port
  BTSerial.begin(9600);

  Serial.println("Bluetooth Test: Ready to receive and send data");
}

void loop() {
  // Check if data is available from the Bluetooth module
  if (BTSerial.available()) {
    char receivedChar = BTSerial.read(); // Read a character from Bluetooth
    Serial.print("Received via Bluetooth: ");
    Serial.println(receivedChar);

    // Echo the received character back to the Bluetooth app
    BTSerial.print("Echo: ");
    BTSerial.println(receivedChar);
  }

  // Check if data is available from the Serial Monitor
  if (Serial.available()) {
    char sendChar = Serial.read(); // Read a character from Serial Monitor
    BTSerial.write(sendChar); // Send the character via Bluetooth
  }
}