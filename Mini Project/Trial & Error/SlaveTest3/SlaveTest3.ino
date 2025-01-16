#include <Wire.h>

// Define the LM35 sensor pin and the stop button pin
const int tempPin = A0;
const int buttonPin = 2;  // Pin connected to the button
const int slaveAddress = 8; // I²C address of this slave

volatile bool buttonPressed = false; // Flag to indicate button press

void setup() {
    Wire.begin(slaveAddress); // Join I²C bus with slave address
    Wire.onRequest(requestEvent); // Register function to send data
    pinMode(tempPin, INPUT);
    pinMode(buttonPin, INPUT_PULLUP); // Button with pull-up resistor
    attachInterrupt(digitalPinToInterrupt(buttonPin), handleButtonPress, FALLING); // Interrupt on button press
    Serial.begin(9600); // For debugging
}

// Interrupt Service Routine (ISR) to handle button press
void handleButtonPress() {
    buttonPressed = true; // Set the flag
    Serial.println("Button pressed. Sending stop signal to master.");
}

// Function to read temperature from LM35
float readTemperature() {
    int sensorValue = analogRead(tempPin);
    float voltage = sensorValue * (5.0 / 1023.0); // Convert to voltage
    float temperature = voltage * 100; // LM35 outputs 10 mV/°C
    return temperature;
}

// Function to send data to the master
void requestEvent() {
    if (buttonPressed) {
        Wire.write('S'); // Send stop signal to the master
        buttonPressed = false; // Reset the flag
    } else {
        float temp = readTemperature();
        int tempInt = static_cast<int>(temp * 100); // Send temperature as an integer (scaled by 100)
        Wire.write((tempInt >> 8) & 0xFF); // Send high byte
        Wire.write(tempInt & 0xFF);       // Send low byte
        Serial.print("Sent temperature to master: ");
        Serial.println(temp);
    }
}

void loop() {
    // The slave operates passively, waiting for master requests or button presses
}
