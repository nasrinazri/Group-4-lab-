#include <Wire.h>
#include <HX711_ADC.h>

const int tempPin = A0;           // LM35 sensor pin
const int waterLevelPin = A1;     // Water level sensor pin
const int slaveAddress = 8;       // I²C address of this slave

// Load cell pins
const int HX711_dout = 4;        // HX711 dout pin
const int HX711_sck = 5;         // HX711 sck pin

// Load cell constructor
HX711_ADC LoadCell(HX711_dout, HX711_sck);

void setup() {
    Wire.begin(slaveAddress);     // Join I²C bus with slave address
    Wire.onRequest(requestEvent); // Register function to send data
    pinMode(tempPin, INPUT);
    pinMode(waterLevelPin, INPUT);
    Serial.begin(9600);          // Using 9600 baud rate
    
    // Initialize load cell
    LoadCell.begin();
    LoadCell.start(2000, true);  // Start with 2000ms stabilizing time and tare
    
    // Check if load cell is properly connected
    if (LoadCell.getTareTimeoutFlag() || LoadCell.getSignalTimeoutFlag()) {
        Serial.println("Timeout, check MCU>HX711 wiring and pin designations");
        while (1);
    }
    
    // Set the calibration value obtained from the calibration process
    LoadCell.setCalFactor(1.0);  // Replace 1.0 with your calibration value
    Serial.println("Startup complete");
}

// Function to read temperature from LM35
float readTemperature() {
    int sensorValue = analogRead(tempPin);
    float voltage = sensorValue * (5.0 / 1023.0);
    float temperature = voltage * 100;
    return temperature;
}

// Function to read water level sensor value
int readWaterLevel() {
    int waterLevelValue = analogRead(waterLevelPin);
    return waterLevelValue;
}

// Function to read load cell value
float readLoadCell() {
    static boolean newDataReady = false;
    float loadCellValue = 0.0;
    
    if (LoadCell.update()) {
        newDataReady = true;
    }
    
    if (newDataReady) {
        loadCellValue = LoadCell.getData();
        newDataReady = false;
    }
    
    return loadCellValue;
}

// Function to send data to the master
void requestEvent() {
    float temp = readTemperature();
    int tempInt = static_cast<int>(temp * 100);     // Temperature scaled by 100
    int waterLevel = readWaterLevel();              // Water level value
    float loadCell = readLoadCell();                // Load cell value
    int loadCellInt = static_cast<int>(loadCell * 100); // Scale load cell value by 100 to preserve decimals
    
    // Send temperature (2 bytes)
    Wire.write((tempInt >> 8) & 0xFF);
    Wire.write(tempInt & 0xFF);
    
    // Send water level (2 bytes)
    Wire.write((waterLevel >> 8) & 0xFF);
    Wire.write(waterLevel & 0xFF);
    
    // Send load cell value (2 bytes)
    Wire.write((loadCellInt >> 8) & 0xFF);
    Wire.write(loadCellInt & 0xFF);
    
    // Debug output to Serial Monitor
    Serial.print("Temperature: ");
    Serial.print(temp);
    Serial.println(" °C");
    
    Serial.print("Water Level: ");
    Serial.println(waterLevel);
    
    Serial.print("Load Cell: ");
    Serial.print(loadCell);
    Serial.println(" units");
    Serial.println("-------------------");
}

void loop() {
    LoadCell.update(); // Update load cell readings
    // The slave operates passively, waiting for master requests
}