#include <SoftwareSerial.h>

// Define RX and TX pins for SoftwareSerial
SoftwareSerial BTSerial(2, 3); // RX | TX

// LM35 settings
#define LM35PIN A0 // Pin connected to LM35 sensor output

void setup() {
  // Initialize Serial Monitor
  Serial.begin(9600);
  // Initialize Bluetooth Serial
  BTSerial.begin(9600);

  Serial.println("LM35 Bluetooth Data Transmission Starting...");
  BTSerial.println("Ready to display LM35 data!");
}

void loop() {
  // Read the analog value from the LM35 sensor
  int analogValue = analogRead(LM35PIN);

  // Convert the analog value to temperature in Celsius
  float voltage = analogValue * (5.0 / 1023.0); // Convert to voltage (0-5V)
  float temperature = voltage * 100.0; // LM35 gives 10mV/°C

  // Display the data on the Serial Monitor
  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.println(" °C");

  // Send the data to the Bluetooth terminal
  BTSerial.print("Temperature: ");
  BTSerial.print(temperature);
  BTSerial.println(" °C");

  // Wait before the next reading
  delay(2000);
}
