#include <Keypad.h>

// Define the keypad's row and column pin connections
const byte ROW_NUM    = 4; // Four rows
const byte COLUMN_NUM = 4; // Four columns

// Define the pin mappings for the rows and columns
char keys[ROW_NUM][COLUMN_NUM] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};

byte pin_rows[ROW_NUM] = {33, 31, 29, 27}; // Change these pin numbers as needed
byte pin_cols[COLUMN_NUM] = {41, 39, 37, 35}; // Change these pin numbers as needed


Keypad keypad = Keypad(makeKeymap(keys), pin_rows, pin_cols, ROW_NUM, COLUMN_NUM);

void setup() {
  Serial.begin(9600); // Start serial communication
  Serial.println("Keypad Test: Press any key...");
}

void loop() {
  // Check if a key is pressed
  char key = keypad.getKey();
  
  if (key) { // If a key is pressed, the variable will not be null
    Serial.print("Key pressed: ");
    Serial.println(key);
  }
}
