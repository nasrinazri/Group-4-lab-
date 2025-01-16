// Slave Code Using UNO R3

#include <Wire.h>

const int tempPin = A0;           // LM35 sensor pin
const int waterLevelPin = A1;     // Water level sensor pin
const int slaveAddress = 8;       // I²C address of this slave

void setup() {
    Wire.begin(slaveAddress);     // Join I²C bus with slave address
    Wire.onRequest(requestEvent); // Register function to send data
    pinMode(tempPin, INPUT);
    pinMode(waterLevelPin, INPUT);
    Serial.begin(9600);           // For debugging
}

// Function to read temperature from LM35
float readTemperature() {
    int sensorValue = analogRead(tempPin);
    float voltage = sensorValue * (5.0 / 1023.0);  // Convert to voltage
    float temperature = voltage * 100;             // LM35 outputs 10 mV/°C
    return temperature;
}

// Function to read water level sensor value
int readWaterLevel() {
    int waterLevelValue = analogRead(waterLevelPin);
    return waterLevelValue;                        // Return raw analog value (0-1023)
}

// Function to send data to the master
void requestEvent() {
    float temp = readTemperature();
    int tempInt = static_cast<int>(temp * 100);    // Temperature scaled by 100
    int waterLevel = readWaterLevel();             // Read water level sensor

    // Send temperature as two bytes
    Wire.write((tempInt >> 8) & 0xFF);             // High byte of temperature
    Wire.write(tempInt & 0xFF);                    // Low byte of temperature

    // Send water level as two bytes
    Wire.write((waterLevel >> 8) & 0xFF);          // High byte of water level
    Wire.write(waterLevel & 0xFF);                 // Low byte of water level

    Serial.print("Sent temperature to master: ");
    Serial.println(temp);
    Serial.print("Sent water level to master: ");
    Serial.println(waterLevel);
}

void loop() {
}
