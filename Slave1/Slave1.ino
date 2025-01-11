#include <Wire.h>
#include <Keypad.h>

// Define keypad setup
const byte ROW_NUM    = 4; // four rows
const byte COLUMN_NUM = 4; // four columns
char keys[ROW_NUM][COLUMN_NUM] = {
  {'1','2','3','4'},
  {'5','6','7','8'},
  {'9','0','D','S'},
  {'A','C','X','X'}
};
byte pin_rows[ROW_NUM] = {9, 8, 7, 6}; // change to your actual row pins
byte pin_column[COLUMN_NUM] = {5, 4, 3, 2}; // change to your actual column pins

Keypad keypad = Keypad(makeKeymap(keys), pin_rows, pin_column, ROW_NUM, COLUMN_NUM);

String numericBuffer = ""; // Buffer to store numeric input
bool isAddingTokens = false; // State to track if the user is in "ADD tokens" mode

void setup() {
  Wire.begin(8); // Initialize as I2C slave with address 8
  Wire.onRequest(requestEvent); // Register the I2C request handler
  Serial.begin(9600);
  logToBT("Keypad Slave Initialized");
}

void loop() {
  handleKeypadInput();
}

void handleKeypadInput() {
  char key = keypad.getKey(); // Get the key pressed on the keypad

  if (key) {
    if (isAddingTokens) {
      // Handle numeric input for ADD mode
      if (key >= '0' && key <= '9') {
        numericBuffer += key; // Append digit to the buffer
      } else if (key == '#') {
        // Confirm input with #
        Wire.beginTransmission(8);
        Wire.write(numericBuffer.c_str()); // Send numeric input as a string
        Wire.endTransmission();
        isAddingTokens = false;
        numericBuffer = ""; // Clear the buffer after sending
      } else if (key == '*') {
        // Cancel input with *
        numericBuffer = ""; // Clear buffer
        isAddingTokens = false; // Exit ADD mode
      }
    } else {
      // Handle regular command keys
      switch (key) {
        case 'S':  // Start command
          Wire.beginTransmission(8);
          Wire.write("START");
          Wire.endTransmission();
          break;

        case 'A':  // Add tokens command
          isAddingTokens = true; // Enter ADD tokens mode
          break;

        case 'C':  // Check tokens command
          Wire.beginTransmission(8);
          Wire.write("CHECK");
          Wire.endTransmission();
          break;

        default:
          break;
      }
    }
  }
}

// Function to handle I2C request from the Master
void requestEvent() {
  Wire.write("READY");
}