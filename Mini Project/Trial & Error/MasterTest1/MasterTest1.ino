#include <SPI.h>
#include <MFRC522.h>
#include <Servo.h>
#include <Wire.h>

Servo washingMachineServo;

// Pin definitions
#define RST_PIN 5 // Reset pin for RFID module
#define SS_PIN 53  // Slave select pin for RFID module
#define WASH_MACHINE_PIN 7 // Pin to control washing machine
#define SLAVE_ADDRESS 0x08 // Define the slave device I²C address
#define BT_BAUD_RATE 9600  // Baud rate for Bluetooth communication
#define EMERGENCY_STOP_PIN 2 // Emergency stop button pin


MFRC522 rfid(SS_PIN, RST_PIN);
MFRC522::MIFARE_Key key;

// Melody
const int buzzerPin = 8;   // Pin connected to the buzzer
const int buttonPin = 2;   // Pin connected to the button
// Start melody and rhythm
int startMelody[] = {440, 494, 523, 587, 659, 0, 659, 587, 523, 494, 440}; // Notes (A4, B4, C5, D5, E5, pause, E5, D5, C5, B4, A4)
int startNoteDurations[] = {500, 500, 500, 500, 500, 275, 500, 500, 500, 500, 500}; // Durations in ms
// Stop melody and rhythm (higher notes A5 and above)
int stopMelody[] = {880, 988, 1047, 1175, 1319}; // Notes (A5, B5, C6, D6, E6)
int stopNoteDurations[] = {300, 300, 300, 300, 300}; // Durations in ms
bool isPlaying = false;       // State to track if the system is playing
bool buttonPressed = false;   // State to debounce button press

// Variable to track if a card is authenticated
bool isCardAuthenticated = false;

void setup() {
    pinMode(buzzerPin, OUTPUT); 
    pinMode(EMERGENCY_STOP_PIN, INPUT_PULLUP); 

    Wire.begin(); // Join I²C bus as a master
    Serial.begin(9600);
    Serial1.begin(BT_BAUD_RATE); // Initialize Bluetooth communication
    SPI.begin();
    rfid.PCD_Init();

    washingMachineServo.attach(WASH_MACHINE_PIN);
    washingMachineServo.write(0); // Initial position, simulating OFF state

    // Initialize RFID key
    for (byte i = 0; i < 6; i++) {
        key.keyByte[i] = 0xFF;
    }

    logToBT("I²C Master Initialized.");
    logToBT("Place your RFID card to start...");
}

void loop() {
    if (!isCardAuthenticated) {
        waitForCard();
    } else {
        if (Serial.available() > 0 || Serial1.available() > 0) {
            String command;
            if (Serial.available() > 0) {
                command = Serial.readStringUntil('\n');
            } else if (Serial1.available() > 0) {
                command = Serial1.readStringUntil('\n');
            }
            
            command.trim(); // Remove whitespace and newline

            if (command.startsWith("ADD")) {
                int tokensToAdd = command.substring(3).toInt();
                if (tokensToAdd > 0) {
                    addTokens(tokensToAdd);
                } else {
                    logToBT("Invalid token amount. Use 'ADD <number>'.");
                }
            }
             else if (command.equalsIgnoreCase("CHECK")) {
                checkTokens();
            } else if (command.equalsIgnoreCase("START")) {
                useMachine();
            }
              else {
                logToBT("Invalid command. Please try one of the following:");
                printCommands();
            }
        }
    }
}

// Function to log messages to Serial and Bluetooth
void logToBT(String message) {
    Serial.println(message);     // Send to Serial Monitor
    Serial1.println(message);    // Send to Bluetooth
}

void waitForCard() {
    if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
        String uid = "Card detected! UID: ";
        for (byte i = 0; i < rfid.uid.size; i++) {
            uid += String(rfid.uid.uidByte[i], HEX) + " ";
        }
        logToBT(uid);

        isCardAuthenticated = true;
        logToBT("Card authenticated. You can now use the system.");
        printCommands();

        rfid.PICC_HaltA();
        rfid.PCD_StopCrypto1();
    }
}

void printCommands() {
    logToBT("Commands:");
    logToBT("1. Add tokens (ADD): ");
    logToBT("2. Check tokens (CHECK): ");
    logToBT("3. Start System (START): ");
}

void checkTokens() {
    if (!authenticateCard()) return;

    byte blockAddr = 1; 
    byte buffer[18];
    byte bufferSize = sizeof(buffer);
    if (rfid.MIFARE_Read(blockAddr, buffer, &bufferSize) != MFRC522::STATUS_OK) {
        logToBT("Reading failed.");
        rfid.PICC_HaltA();
        return;
    }

    int tokens = buffer[0];
    logToBT("Tokens available: " + String(tokens));

    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
}

void addTokens(int tokensToAdd) {
    if (tokensToAdd <= 0) {
        logToBT("Invalid token amount.");
        return;
    }

    if (!authenticateCard()) return;

    byte blockAddr = 1;
    byte buffer[18];
    byte bufferSize = sizeof(buffer);
    if (rfid.MIFARE_Read(blockAddr, buffer, &bufferSize) != MFRC522::STATUS_OK) {
        logToBT("Reading failed.");
        rfid.PICC_HaltA();
        return;
    }

    int currentTokens = buffer[0];
    int newTokenBalance = currentTokens + tokensToAdd;
    buffer[0] = newTokenBalance;

    if (rfid.MIFARE_Write(blockAddr, buffer, 16) == MFRC522::STATUS_OK) {
        logToBT("Added tokens. New balance: " + String(newTokenBalance));
    } else {
        logToBT("Failed to update token count.");
    }

    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
}

void useMachine() {
    if (!authenticateCard()) return;

    byte blockAddr = 1; // Block address where tokens are stored
    byte buffer[18];
    byte bufferSize = sizeof(buffer);

    // Read current token balance
    if (rfid.MIFARE_Read(blockAddr, buffer, &bufferSize) != MFRC522::STATUS_OK) {
        logToBT("Reading failed.");
        rfid.PICC_HaltA();
        return;
    }

    int tokens = buffer[0]; // Current token count
    logToBT("Tokens available: " + String(tokens));

    if (tokens > 0) {
        // Prompt the user for desired washing time
        logToBT("Enter washing time in seconds (min: 10, increments of 10): ");
        int washingTime = getInputFromUser(); // Function to handle user input

        // Validate washing time
        if (washingTime < 10 || washingTime % 10 != 0) {
            logToBT("Invalid washing time. Please enter a multiple of 10 seconds.");
            return;
        }

        int requiredTokens = 1 + ((washingTime - 10) / 10); // Calculate required tokens
        logToBT("Tokens required: " + String(requiredTokens));

        if (tokens >= requiredTokens) {
            tokens -= requiredTokens; // Deduct the required tokens
            buffer[0] = tokens; // Update buffer with the new token count

            if (rfid.MIFARE_Write(blockAddr, buffer, 16) == MFRC522::STATUS_OK) {
                logToBT("Tokens deducted. Remaining tokens: " + String(tokens));
                logToBT("Starting washing machine for " + String(washingTime) + " seconds...");

                playMelody(startMelody, startNoteDurations, 11);

                // Simulate washing machine operation
                unsigned long startTime = millis();
                while (millis() - startTime < (unsigned long)washingTime * 1000) {
                    washingMachineServo.write(90);  // Move servo to 90 degrees
                    delay(500);
                    washingMachineServo.write(0);   // Move servo back to 0 degrees
                    delay(500);
                }

                logToBT("Washing complete. Thank you!");
                printCommands();
            } else {
                logToBT("Failed to update token count.");
            }
        } else {
            logToBT("Not enough tokens. Required: " + String(requiredTokens) + ", Available: " + String(tokens));
        }
    } else {
        logToBT("Not enough tokens.");
    }

    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
}


bool authenticateCard() {
    if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) {
        logToBT("No card detected.");
        return false;
    }

    byte blockAddr = 1; // Block address where tokens are stored
    if (rfid.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, blockAddr, &key, &(rfid.uid)) != MFRC522::STATUS_OK) {
        logToBT("Authentication failed.");
        rfid.PICC_HaltA();
        return false;
    }

    return true;
}

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

void playTone(int frequency, int duration) {
    int period = 1000000 / frequency; // Calculate the period in microseconds
    int pulse = period / 2;           // Calculate the pulse width
    unsigned long endTime = millis() + duration;
    while (millis() < endTime) {
        digitalWrite(buzzerPin, HIGH);
        delayMicroseconds(pulse);
        digitalWrite(buzzerPin, LOW);
        delayMicroseconds(pulse);
    }
}

void sendToSlave(String message) {
    Wire.beginTransmission(SLAVE_ADDRESS); // Start transmission to slave
    Wire.write(message.c_str());          // Send the message
    Wire.endTransmission();               // End transmission
    Serial.println("Sent to slave: " + message);
}

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
        return value; // Return valid input
    } else {
        logToBT("Invalid input. Please enter a number.");
        return getInputFromUser(); // Retry if input is invalid
    }
}

