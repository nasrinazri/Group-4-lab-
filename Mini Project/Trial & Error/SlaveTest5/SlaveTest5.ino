#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Define the LM35 sensor pin and the stop button pin
const int tempPin = A0;
const int slaveAddress = 8; // I²C address of this slave

// Initialize the LCD (address 0x27 and 16x2 size)
LiquidCrystal_I2C lcd(0x27, 20, 4);

void setup() {
    Wire.begin(slaveAddress); // Join I²C bus with slave address
    Wire.onRequest(requestEvent); // Register function to send data
    Wire.onReceive(receiveEvent); // Register function to receive data
    pinMode(tempPin, INPUT);
    Serial.begin(9600); // For debugging
    lcd.begin(20,4);       // Initialize the LCD
    lcd.backlight();   // Turn on the LCD backlight
    lcd.clear();       // Clear any existing text on the LCD
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
    float temp = readTemperature();
    int tempInt = static_cast<int>(temp * 100); // Send temperature as an integer (scaled by 100)
    Wire.write((tempInt >> 8) & 0xFF); // Send high byte
    Wire.write(tempInt & 0xFF);       // Send low byte
    Serial.print("Sent temperature to master: ");
    Serial.println(temp);
}

// Function to handle data received from master
void receiveEvent(int numBytes) {
    lcd.clear(); // Clear LCD before displaying new message
    String message = ""; // Buffer for the received data
    
    while (Wire.available()) {
        char c = Wire.read(); // Read each byte from the master
        message += c; // Append it to the message string
    }
    
    // Display the received message on the LCD and in Serial Monitor
    Serial.print("Message from master: ");
    Serial.println(message);
    lcd.setCursor(0, 0); // Start at the first row and column of the LCD
    lcd.print("Master Msg:"); 
    lcd.setCursor(0, 1); // Move to the second row of the LCD
    lcd.print(message);
}

void loop() {
    // The slave operates passively, waiting for master requests or button presses
}
