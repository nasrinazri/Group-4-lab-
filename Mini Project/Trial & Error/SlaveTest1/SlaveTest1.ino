#include <Wire.h>

void setup() {
    Wire.begin(8);               // Initialize I²C as Slave with address 8
    Wire.onReceive(receiveEvent); // Register receive event
    Serial.begin(9600);          // For Serial Monitor
}

void loop() {
    // The main loop is empty because the slave reacts to events
}

void receiveEvent(int numBytes) {
    String message = "";

    // Read the incoming message from the Master
    while (Wire.available()) {
        char c = Wire.read();
        message += c;
    }

    // Print the received message to the Serial Monitor
    Serial.println("Message from Master: " + message);
}

