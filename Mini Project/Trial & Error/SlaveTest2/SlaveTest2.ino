#include <Wire.h>
#include <LiquidCrystal_I2C.h>  // Include the library for the I²C LCD

// Initialize the LCD. Adjust address (e.g., 0x27 or 0x3F) if needed for your module.
LiquidCrystal_I2C lcd(0x27, 20, 4); 

void setup() {
    Wire.begin(8);                   // Initialize I²C as Slave with address 8
    Wire.onReceive(receiveEvent);    // Register receive event
    Serial.begin(9600);              // Initialize Serial Monitor

    lcd.init();                      // Initialize the LCD
    lcd.backlight();                 // Turn on the LCD backlight
    lcd.clear();                     // Clear the LCD screen
    lcd.setCursor(0, 0);             // Set cursor to the top-left position
    lcd.print("Waiting for data");   // Display initial message
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

    // Display the received message on the Serial Monitor
    Serial.println("Message from Master: " + message);

    // Clear the LCD and display the received message
    lcd.clear();
    lcd.setCursor(0, 0);             // Start from the first line, first column
    lcd.print("Received:");          
    lcd.setCursor(0, 1);             // Move to the second line
    lcd.print(message);

    // Optional: Handle scrolling or multi-line messages
    if (message.length() > 20) {    // If message exceeds one row length (20 characters)
        lcd.setCursor(0, 2);         // Move to the third line
        lcd.print(message.substring(20, 40));
        lcd.setCursor(0, 3);         // Move to the fourth line if more content exists
        lcd.print(message.substring(40));
    }
}
