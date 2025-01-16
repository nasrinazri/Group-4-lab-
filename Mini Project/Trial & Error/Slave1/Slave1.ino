#include <Wire.h>
#include <Keypad.h>

// Keypad setup
const byte ROW_NUM    = 4; // Four rows
const byte COLUMN_NUM = 4; // Four columns
char keys[ROW_NUM][COLUMN_NUM] = {
  {'1','2','3','4'},
  {'5','6','7','8'},
  {'9','0','D','S'},
  {'A','C','X','X'}
};
byte pin_rows[ROW_NUM] = {9, 8, 7, 6}; // Row pins
byte pin_column[COLUMN_NUM] = {5, 4, 3, 2}; // Column pins

Keypad keypad = Keypad(makeKeymap(keys), pin_rows, pin_column, ROW_NUM, COLUMN_NUM);

// I2C Slave address
#define SLAVE_ADDR 8

void setup() {
  Wire.begin(SLAVE_ADDR); // I2C Slave address
  Wire.onRequest(requestEvent); // Register the request handler
  Serial.begin(9600);
}

void loop() {
  char key = keypad.getKey(); // Check if a key is pressed
  if (key) {
    // You can implement specific actions based on keypad keys
    Serial.print("Key pressed: ");
    Serial.println(key);

    // Send key press to master via I2C when key is pressed
    Wire.beginTransmission(SLAVE_ADDR);
    Wire.write(key); // Send key press as a byte
    Wire.endTransmission();
  }
}

// I2C request handler function
void requestEvent() {
  Wire.write("Ready"); // Send a response to the master when it requests
}
