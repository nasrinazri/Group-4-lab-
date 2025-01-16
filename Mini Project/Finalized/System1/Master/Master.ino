// Master Code using Arduino AT Mega 2560

// Arduino IDE version 2.3.4

#include <SPI.h>
#include <MFRC522.h>      
#include <Servo.h>
#include <Wire.h>
#include <Keypad.h>
#include <LiquidCrystal_I2C.h>

// Initialize Servo
Servo washingMachineServo;

// Pin definitions
#define SLAVE_ADDRESS 0x08  // I²C address for slave
#define BT_BAUD_RATE 9600   // Baud rate for Bluetooth communication

#define GREEN_LED_PIN 26    // Green LED for "Ready"
#define YELLOW_LED_PIN 24   // Yellow LED for "Washing"
#define RED_LED_PIN 22      // Red LED for "Emergency Stop"
#define RST_PIN 47          // Reset pin for RFID module
#define SS_PIN 53           // Slave select pin for RFID module
#define WASH_MACHINE_PIN 7  // Pin to control washing machine
#define STOP_BUTTON_PIN 2   // Digital pin connected to the stop button

// Global variables
volatile bool stopFlag = false;   // Flag to detect emergency stop
bool isCardAuthenticated = false; // Tracks card authentication state
MFRC522 rfid(SS_PIN, RST_PIN);
MFRC522::MIFARE_Key key;

// Buzzer and melody settings
#define BUZZER_PIN 8
#define BUZZER_PIN_2 5

const int startMelody[] = {440, 494, 523, 587, 659, 0, 659, 587, 523, 494, 440};
const int startNoteDurations[] = {500, 500, 500, 500, 500, 275, 500, 500, 500, 500, 500};

const int stopMelody[] = {880, 988, 1047, 1175, 1319};
const int stopNoteDurations[] = {300, 300, 300, 300, 300};

// Keypad configuration
const byte ROWS = 4; // Number of rows
const byte COLS = 4; // Number of columns

char keys[ROWS][COLS] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}
};

byte rowPins[ROWS] = {33, 31, 29, 27}; // Row pins
byte colPins[COLS] = {41, 39, 37, 35}; // Column pins

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);



void setup() {
    // Pin configurations
    pinMode(BUZZER_PIN, OUTPUT);
    pinMode(BUZZER_PIN_2, OUTPUT);
    pinMode(STOP_BUTTON_PIN, INPUT_PULLUP); // Enable internal pull-up resistor
    attachInterrupt(digitalPinToInterrupt(STOP_BUTTON_PIN), stopButtonISR, FALLING); // Interrupt on button press

    pinMode(GREEN_LED_PIN, OUTPUT);
    pinMode(YELLOW_LED_PIN, OUTPUT);
    pinMode(RED_LED_PIN, OUTPUT);

    // Set initial LED states: Indicate system is ready at startup
    digitalWrite(GREEN_LED_PIN, HIGH); 
    digitalWrite(YELLOW_LED_PIN, LOW);
    digitalWrite(RED_LED_PIN, LOW);

    // Initialize communication protocols
    Wire.begin();                // Join I²C bus as a master
    Serial.begin(9600);          // Initialize Serial Connection
    Serial1.begin(BT_BAUD_RATE); // Initialize Bluetooth communication
    SPI.begin();                 // Initialize SPI protocol
    rfid.PCD_Init();

    // Initialize Servo
    washingMachineServo.attach(WASH_MACHINE_PIN);
    washingMachineServo.write(0); // Initial position, simulating OFF state

    // Initialize RFID key
    for (byte i = 0; i < 6; i++) {
        key.keyByte[i] = 0xFF;
    }

    // Log startup messages
    logToBT("I²C Master Initialized.");
    logToBT("Place your RFID card to start...");
}

void loop() {
    if (!isCardAuthenticated) {
        indicateSystemReady();
        waitForCard();
    } else {
        char key = keypad.getKey();    // Check for input from the keypad

        if (key) {
            processKeypadCommand(key); // Process the key pressed
        }

        // Check for input from Serial or Bluetooth
        if (Serial.available() > 0 || Serial1.available() > 0) {
            String command;
            if (Serial.available() > 0) {
                command = Serial.readStringUntil('\n');
            } else if (Serial1.available() > 0) {
                command = Serial1.readStringUntil('\n');
            }
            command.trim();
            processCommand(command);
        }
    }
}

// Indicator: System Ready
void indicateSystemReady() {
    digitalWrite(GREEN_LED_PIN, HIGH);  // Green LED only On
    digitalWrite(YELLOW_LED_PIN, LOW);
    digitalWrite(RED_LED_PIN, LOW);
}

// Indicator: Washing State
void indicateWashingState() {
    digitalWrite(GREEN_LED_PIN, LOW);
    digitalWrite(YELLOW_LED_PIN, HIGH); // Yellow LED only On
    digitalWrite(RED_LED_PIN, LOW);
}

// Indicator: Emergency
void indicateEmergencyStop() {
    digitalWrite(GREEN_LED_PIN, LOW);
    digitalWrite(YELLOW_LED_PIN, LOW);
    digitalWrite(RED_LED_PIN, HIGH);  // Red LED only ON

    delay(10000); // delay a bit

    indicateSystemReady();  // Turn on Green LED
}

// Verify Stop Button 
void stopButtonISR() {
    static unsigned long lastInterruptTime = 0;
    unsigned long interruptTime = millis();

    // Debounce check (ignore if within 200ms of the last interrupt)
    if (interruptTime - lastInterruptTime > 200) {
        stopFlag = true;  
        indicateEmergencyStop();                // Turn on red LED
        logToBT("Emergency stop activated.");   // Set flag to stop the machine
    }
    lastInterruptTime = interruptTime; 
}

// Melody for Start
void playMelody(int melody[], int noteDurations[], int length) {
    for (int i = 0; i < length; i++) {
        int noteDuration = noteDurations[i];
        if (melody[i] == 0) {
            delay(noteDuration);  // Pause for the duration if note is 0
        } else {
            playTone(melody[i], noteDuration);
        }
    }
}

// Tone for Start
void playTone(int frequency, int duration) {
    int period = 1000000 / frequency;   // Calculate the period in microseconds
    int pulse = period / 2;             // Calculate the pulse width
    unsigned long endTime = millis() + duration;

    while (millis() < endTime) {
        digitalWrite(BUZZER_PIN, HIGH); // Turn on Buzzer
        delayMicroseconds(pulse);
        digitalWrite(BUZZER_PIN, LOW);  // Turn off Buzzer
        delayMicroseconds(pulse);
    }
}

// Melody for Stop
void playMelody2(int melody[], int noteDurations[], int length) {
    for (int i = 0; i < length; i++) {
        int noteDuration = noteDurations[i];
        if (melody[i] == 0) {
            delay(noteDuration);  // Pause for the duration if note is 0
        } else {
            playTone2(melody[i], noteDuration);
        }
    }
}

// Tone for Stop
void playTone2(int frequency, int duration) {
    int period = 1000000 / frequency; // Calculate the period in microseconds
    int pulse = period / 2;           // Calculate the pulse width
    unsigned long endTime = millis() + duration;
    while (millis() < endTime) {
        digitalWrite(BUZZER_PIN_2, HIGH);   // Turn on Buzzer
        delayMicroseconds(pulse);
        digitalWrite(BUZZER_PIN_2, LOW);    // Turn off Buzzer
        delayMicroseconds(pulse);
      }
}

// log Messages to Serial and Bluetooth
void logToBT(String message) {
    Serial.println(message);     // Send to Serial Monitor
    Serial1.println(message);    // Send to Bluetooth
}

// Sending Message to Slave (Unused) 
void sendToSlave(String message) {
    Wire.beginTransmission(SLAVE_ADDRESS);    // Start transmission to slave
    Wire.write(message.c_str());              // Send the message
    Wire.endTransmission();                   // End transmission
    Serial.println("Sent to slave: " + message);
}

// Check Token changes from slave (Unused)
int getTokensFromSlave() {
    // Get token amount from the slave via I2C
    String tokenAmount = "";
    Wire.requestFrom(8, 32);
    while (Wire.available()) {
      char received = Wire.read();
      tokenAmount += received;
    }
    return tokenAmount.toInt();
}

// Print Avalaible Commands
void printCommands() {
    logToBT("Commands:");
    logToBT("1. Add tokens (ADD): ");
    logToBT("2. Check tokens (CHECK): ");
    logToBT("3. Start System (START): ");
    logToBT("4. Maintanence Mode (FIX): ");

}

// Process User Command from BT or Serial
void processCommand(String command) {
    // Checking for user input if it correct then assined to different function
    if (command.startsWith("ADD")) {
        addTokens();
    } else if (command.equalsIgnoreCase("CHECK")) {
        checkTokens();
    } else if (command.equalsIgnoreCase("START")) {
        useMachine();
    } else if (command.equalsIgnoreCase("FIX")) {
        fix();
    } else {
        logToBT("Invalid command. Please try one of the following:");
        printCommands();
    }
}

// Process User Command from Keypad
void processKeypadCommand(char key) {
    if (key == 'A') {                                          // A: Add tokens
        logToBT("Keypad input: A (Add Tokens)");
        logToBT("Enter the number of tokens to add:");
        int tokensToAdd = getNumericInputFromKeypad();        // Get numeric input from the keypad
        if (tokensToAdd > 0) {
            logToBT("Adding tokens: " + String(tokensToAdd)); // Adding token
            addTokensFromKeypad(tokensToAdd);                 // Update token
        } else {
            logToBT("Invalid input for tokens.");
        }

    } else if (key == 'B') {                                  // B: Check tokens
        logToBT("Keypad input: B (Check Tokens)");
        checkTokens();

    } else if (key == 'C') {                                  // C: Start the washing machine
        logToBT("Keypad input: C (Start Machine)");
        logToBT("Select washing temperature:");
        logToBT("1. Hot (3 tokens)");
        logToBT("2. Warm (2 tokens)");
        logToBT("3. Cold (1 token)");
        int tempChoice = getNumericInputFromKeypad();         // Get numeric input from the keypad
        if (tempChoice < 1 || tempChoice > 3) {
            logToBT("Invalid temperature choice.");
            return;
        }

        logToBT("Enter washing time in seconds (min: 10, increments of 10):");  // Adding more time with incriment of 10 = 1 Token
        int washingTime = getNumericInputFromKeypad();
        if (washingTime < 10 || washingTime % 10 != 0) {
            logToBT("Invalid washing time.");
            return;
        }

        logToBT("Starting machine with temperature choice: " + String(tempChoice) + " and time: " + String(washingTime));
        startMachineWithKeypad(tempChoice, washingTime);

    } else if (key == 'D') {                                  // D: Maintenance mode
        logToBT("Keypad input: D (Fix)");
        fix();

    } else {  // Invalid key
        logToBT("Invalid key pressed on keypad. Use A, B, C, or D."); // Invalid
    }
}

// Process Numeric Input
int getNumericInputFromKeypad() {
    String input = "";        // Buffer to store numeric input
    char key;

    logToBT("Enter a number:");

    // Wait for numeric input from keypad
    while (true) {
        key = keypad.getKey();
        if (key) {
            if (isdigit(key)) {
                input += key;                 // Append digit to input buffer
                logToBT("Entered: " + input); // Log Input from user to Serial & BT
            } else if (key == '#') {          // '#' = For Entering Input
                break;
            } else {
                logToBT("Invalid key. Enter digits only, end with '#'.");
            }
        }
    }

    return input.toInt();                     // Convert the input string to an integer: System Using Integer not String
}

// Read Card Serial 
void waitForCard() {
    if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) { //Reading Card UID
        String uid = "Card detected! UID: ";
        for (byte i = 0; i < rfid.uid.size; i++) {
            uid += String(rfid.uid.uidByte[i], HEX) + " ";
        }
        logToBT(uid);   // Log the UID

        isCardAuthenticated = true;
        logToBT("Card authenticated. You can now use the system.");
        printCommands();

        rfid.PICC_HaltA();
        rfid.PCD_StopCrypto1();
      }
}

// Authenticate card and store the token value
bool authenticateCard() {
    if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) {
        logToBT("No card detected.");
        return false;
    }

    byte blockAddr = 1;     // Block address where tokens are stored
    if (rfid.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, blockAddr, &key, &(rfid.uid)) != MFRC522::STATUS_OK) {
        logToBT("Authentication failed.");
        rfid.PICC_HaltA();
        return false;
    }

    return true;
}

// Check user Input from BT Terminal & Serial Monitor
int getInputFromUser() {
    String input = "";
    while (true) {
        // Check Serial input (Serial Monitor)
        if (Serial.available()) {
            input = Serial.readStringUntil('\n');
            input.trim();
            break;
        }

        // Check Serial1 input (Bluetooth terminal)
        if (Serial1.available()) {
            input = Serial1.readStringUntil('\n');
            input.trim();
            break;
        }
    }

    int value = input.toInt(); // Convert input to integer
    if (value > 0) {
        return value;          // Return valid input
    } else {
        logToBT("Invalid input. Please enter a number.");
        return getInputFromUser(); // Retry if input is invalid
    }
}

// Checking Token inside the card
void checkTokens() {
    if (!authenticateCard()) return; // Check card authentication first

    byte blockAddr = 1;              
    byte buffer[18];
    byte bufferSize = sizeof(buffer);
    if (rfid.MIFARE_Read(blockAddr, buffer, &bufferSize) != MFRC522::STATUS_OK) {
        logToBT("Reading failed.");
        rfid.PICC_HaltA();
        return;
    }

    int tokens = buffer[0];   // Token inside buffer[0]
    logToBT("Tokens available: " + String(tokens));

    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
}

// Adding Token inside the card: From BT or Serial
void addTokens() {
    if (!authenticateCard()) return; // Authenticate the card first

    byte blockAddr = 1;
    byte buffer[18];    
    byte bufferSize = sizeof(buffer);

    // Read the current token balance
    if (rfid.MIFARE_Read(blockAddr, buffer, &bufferSize) != MFRC522::STATUS_OK) {
        logToBT("Reading failed.");
        rfid.PICC_HaltA();
        return;
    }

    // Print Current token by checking inside Buffer[0]
    int currentTokens = buffer[0];
    logToBT("Current tokens: " + String(currentTokens));

    // Ask the user to input the token amount
    logToBT("Enter the number of tokens to add:");
    int tokensToAdd = getInputFromUser();

    // Token must exist in order to used
    if (tokensToAdd <= 0) {
        logToBT("Invalid token amount. Please enter a positive number.");
        return;
    }

    // Update the token balance
    int newTokenBalance = currentTokens + tokensToAdd;
    buffer[0] = newTokenBalance;

    // Adding updated token value to the card
    if (rfid.MIFARE_Write(blockAddr, buffer, 16) == MFRC522::STATUS_OK) {
        logToBT("Tokens successfully added. New balance: " + String(newTokenBalance));
    } else {
        logToBT("Failed to update token count.");
    }

    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();

    printCommands();
}

// Adding Token inside the card: From Keypad
void addTokensFromKeypad(int tokensToAdd) {
    if (!authenticateCard()) return;    // Authenticate the card first

    byte blockAddr = 1;
    byte buffer[18];
    byte bufferSize = sizeof(buffer);

    // Read the current token balance
    if (rfid.MIFARE_Read(blockAddr, buffer, &bufferSize) != MFRC522::STATUS_OK) {
        logToBT("Reading failed.");
        rfid.PICC_HaltA();
        return;
    }

    int currentTokens = buffer[0];
    int newTokenBalance = currentTokens + tokensToAdd;

    buffer[0] = newTokenBalance;

    if (rfid.MIFARE_Write(blockAddr, buffer, 16) == MFRC522::STATUS_OK) {
        logToBT("Tokens successfully added. New balance: " + String(newTokenBalance));
    } else {
        logToBT("Failed to update token count.");
    }

    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();

    printCommands();
}

// Mantainence: Check Temp & Water Level from Slave
void fix() {
    Wire.beginTransmission(8); // Start communication with slave at address 8
    Wire.write('F');           // Send the 'Fix' command to the slave
    Wire.endTransmission();

    delay(100);                // Wait for slave to prepare the data

    Wire.requestFrom(8, 4);    // Request 4 bytes (2 for temperature, 2 for water level)
    if (Wire.available() == 4) {
        // Read temperature bytes
        int highByteTemp = Wire.read();
        int lowByteTemp = Wire.read();
        float temperature = (highByteTemp << 8 | lowByteTemp) / 100.0;

        // Read water level bytes
        int highByteWater = Wire.read();
        int lowByteWater = Wire.read();
        int waterLevel = (highByteWater << 8 | lowByteWater);

        // Display temperature and water level
        Serial.print("Temperature received from slave: ");
        Serial.print(temperature);
        Serial.println(" °C");

        Serial.print("Water level received from slave: ");
        Serial.println(waterLevel);

        logToBT("Current temperature: " + String(temperature) + " °C");
        logToBT("Current water level: " + String(waterLevel));

    } else {
        Serial.println("Error: Could not retrieve data from slave.");
    }

    printCommands();
}

// Main Function : From BT or Serial 
void useMachine() {
    stopFlag = false;                       // Reset the stop flag to ensure smooth operation

    if (!authenticateCard()) return;        // Authenticate the card. If failed, exit.

    byte blockAddr = 1;                     // Memory block address in RFID to read/write data
    byte buffer[18];                        // Buffer to hold data read from the card
    byte bufferSize = sizeof(buffer);       // Size of the buffer

    // Attempt to read data from the RFID card
    if (rfid.MIFARE_Read(blockAddr, buffer, &bufferSize) != MFRC522::STATUS_OK) {
        logToBT("Reading failed.");         // Log failure if unable to read
        rfid.PICC_HaltA();                  // Halt RFID communication
        return;
    }

    indicateWashingState();                 // Turn on Washing State LED

    int tokens = buffer[0];                 // Retrieve the number of tokens from the RFID card
    logToBT("Tokens available: " + String(tokens));  // Log available tokens

    // 🧼 Step 1: Ask user to choose the washing temperature
    logToBT("Select washing temperature:");
    logToBT("1. Hot (3 tokens)");
    logToBT("2. Warm (2 tokens)");
    logToBT("3. Cold (1 token)");

    int tempChoice = getInputFromUser();    // Get user input for temperature choice
    int tempTokens = 0;                     // Tokens required for the selected temperature

    // Process user's temperature choice
    switch (tempChoice) {
        case 1:
            logToBT("You selected: Hot");
            tempTokens = 3;                 // Deduct 3 tokens for hot wash
            break;
        case 2:
            logToBT("You selected: Warm");
            tempTokens = 2;                 // Deduct 2 tokens for warm wash
            break;
        case 3:
            logToBT("You selected: Cold");
            tempTokens = 1;                 // Deduct 1 token for cold wash
            break;
        default:
            logToBT("Invalid choice. Please try again.");
            return;                         // Exit if the choice is invalid
    }

    // Check if the user has enough tokens for the selected temperature
    if (tokens < tempTokens) {
        logToBT("Not enough tokens for the selected temperature. Required: " + String(tempTokens) + ", Available: " + String(tokens));
        return;
    }

    // Deduct tokens for temperature selection
    tokens -= tempTokens;
    logToBT("Tokens deducted for temperature: " + String(tempTokens) + ". Remaining tokens: " + String(tokens));

    // 🕒 Step 2: Ask user to enter washing time
    logToBT("Enter washing time in seconds (min: 10, increments of 10): ");
    int washingTime = getInputFromUser();               // Get user input for washing time

    // Validate the washing time entered by the user
    if (washingTime < 10 || washingTime % 10 != 0) {
        logToBT("Invalid washing time. Please enter a multiple of 10 seconds.");
        return;
    }

    int requiredTokens = 1 + ((washingTime - 10) / 10); // Calculate tokens needed for the time
    logToBT("Tokens required for time: " + String(requiredTokens));

    // Check if the user has enough tokens for the selected time
    if (tokens >= requiredTokens) {
        tokens -= requiredTokens;                       // Deduct tokens for the time
        buffer[0] = tokens;                             // Update the buffer with remaining tokens

        // Write updated tokens back to the RFID card
        if (rfid.MIFARE_Write(blockAddr, buffer, 16) == MFRC522::STATUS_OK) {
            logToBT("Tokens deducted. Remaining tokens: " + String(tokens));
            logToBT("Starting washing machine for " + String(washingTime) + " seconds...");

            playMelody(startMelody, startNoteDurations, 11);  // Play start melody

            unsigned long startTime = millis();               // Record the start time
            unsigned long remainingTime = washingTime * 1000; // Convert washing time to milliseconds

            // Operate the washing machine for the specified time
            while (millis() - startTime < remainingTime) {
                if (stopFlag) {                               // Check if the stop flag is triggered
                    logToBT("Operation stopped by user.");
                    break;
                }
                washingMachineServo.write(90);                // Simulate machine operation
                delay(500);
                washingMachineServo.write(0);
                delay(500);
            }

            if (!stopFlag) {                                  // If not stopped prematurely
                indicateSystemReady();                        // Indicate the system is ready
                logToBT("Washing complete. Thank you!");
                playMelody2(stopMelody, startNoteDurations, 5); // Play stop melody
            }

            printCommands();                                  // Print next available commands
        } else {
            logToBT("Failed to update token count."); // Log failure if writing to card fails
        }
    } else {
        logToBT("Not enough tokens. Required: " + String(requiredTokens) + ", Available: " + String(tokens));
    }

    rfid.PICC_HaltA();                                        // Halt RFID communication
    rfid.PCD_StopCrypto1();                                   // Stop cryptographic communication with the card
}

// Main Function : From Keypad
void startMachineWithKeypad(int tempChoice, int washingTime) {
    stopFlag = false;                        // Reset the stop flag

    if (!authenticateCard()) return;         // Authenticate the card. If failed, exit.

    byte blockAddr = 1;                      // Memory block address in RFID to read/write data
    byte buffer[18];                         // Buffer to hold data read from the card
    byte bufferSize = sizeof(buffer);        // Size of the buffer

    // Attempt to read data from the RFID card
    if (rfid.MIFARE_Read(blockAddr, buffer, &bufferSize) != MFRC522::STATUS_OK) {
        logToBT("Reading failed.");          // Log failure if unable to read
        rfid.PICC_HaltA();                   // Halt RFID communication
        return;
    }

    int tokens = buffer[0];                 // Retrieve the number of tokens from the RFID card
    int tempTokens = (tempChoice == 1) ? 3 : (tempChoice == 2) ? 2 : 1;   // Tokens required for temperature choice

    // Check if the user has enough tokens for the selected temperature
    if (tokens < tempTokens) {
        logToBT("Not enough tokens for the selected temperature.");
        return;
    }

    int requiredTokens = 1 + ((washingTime - 10) / 10);                   // Calculate tokens needed for the time
    if (tokens < tempTokens + requiredTokens) {
        logToBT("Not enough tokens for the selected time.");
        return;
    }

    tokens -= (tempTokens + requiredTokens);                              // Deduct tokens for temperature and time
    buffer[0] = tokens;                                                   // Update the buffer with remaining tokens

    // Write updated tokens back to the RFID card
    if (rfid.MIFARE_Write(blockAddr, buffer, 16) == MFRC522::STATUS_OK) {
        logToBT("Tokens deducted. Remaining tokens: " + String(tokens));
        logToBT("Starting washing machine...");
        indicateWashingState();                                          // Turn On Washing State LED  

        playMelody(startMelody, startNoteDurations, 11);                 // Play start melody

        unsigned long startTime = millis();                              // Record the start time
        unsigned long remainingTime = washingTime * 1000;                // Convert washing time to milliseconds

        // Operate the washing machine for the specified time
        while (millis() - startTime < remainingTime) {
            if (stopFlag) {                                             // Check if the stop flag is triggered
                logToBT("Operation stopped by user.");
                break;
            }
            washingMachineServo.write(90);                              // Simulate machine operation
            delay(500);
            washingMachineServo.write(0);
            delay(500);
        }

        if (!stopFlag) {                                                // If not stopped prematurely
            indicateSystemReady();                                      // Indicate the system is ready
            logToBT("Washing complete. Thank you!");
            playMelody2(stopMelody, startNoteDurations, 5);             // Play stop melody
        }
    } else {
        logToBT("Failed to update token count.");                       // Log failure if writing to card fails
    }

    rfid.PICC_HaltA();                                                  // Halt RFID communication
    rfid.PCD_StopCrypto1();                                             // Stop cryptographic communication with the card
}





















