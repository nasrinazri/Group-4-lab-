#include <SPI.h>
#include <MFRC522.h>
#include <Servo.h>
#include <Wire.h>

// Servo and Pin Definitions
Servo washingMachineServo;
#define RST_PIN 5         // Reset pin for RFID module
#define SS_PIN 53         // Slave select pin for RFID module
#define WASH_MACHINE_PIN 7 // Pin to control washing machine
#define SLAVE_ADDRESS 0x08 // Slave I²C address
#define BT_BAUD_RATE 9600  // Baud rate for Bluetooth communication
#define EMERGENCY_STOP_PIN 2 // Emergency stop button pin
#define BUZZER_PIN 8       // Pin connected to the buzzer

// Melody and Rhythm
int startMelody[] = {440, 494, 523, 587, 659, 0, 659, 587, 523, 494, 440}; 
int startNoteDurations[] = {500, 500, 500, 500, 500, 275, 500, 500, 500, 500, 500}; 
int stopMelody[] = {880, 988, 1047, 1175, 1319}; 
int stopNoteDurations[] = {300, 300, 300, 300, 300}; 

// State Tracking
bool isPlaying = false;
bool buttonPressed = false;
bool isCardAuthenticated = false;
bool emergencyStop = false;

// RFID Module
MFRC522 rfid(SS_PIN, RST_PIN);
MFRC522::MIFARE_Key key;

void setup() {
    pinMode(EMERGENCY_STOP_PIN, INPUT_PULLUP); // Emergency stop button with pull-up resistor
    pinMode(BUZZER_PIN, OUTPUT);

    Wire.begin();
    Serial.begin(9600);
    Serial1.begin(BT_BAUD_RATE);
    SPI.begin();
    rfid.PCD_Init();

    washingMachineServo.attach(WASH_MACHINE_PIN);
    washingMachineServo.write(0);

    // Initialize RFID key
    for (byte i = 0; i < 6; i++) {
        key.keyByte[i] = 0xFF;
    }

    logToBT("System Initialized.");
    logToBT("Place your RFID card to start...");
}

void loop() {
    // Check for emergency stop
    if (digitalRead(EMERGENCY_STOP_PIN) == LOW && !emergencyStop) {
        emergencyStop = true; // Activate emergency stop
        handleEmergencyStop();
    }

    // Normal operation if no emergency stop
    if (!emergencyStop) {
        if (!isCardAuthenticated) {
            waitForCard();
        } else {
            handleCommands();
        }
    } else {
        // Wait for the emergency stop to reset
        while (digitalRead(EMERGENCY_STOP_PIN) == LOW) {
            // Stay in halt state
        }
        emergencyStop = false;
        logToBT("Emergency Stop Reset. Restart system manually.");
    }
}

// Function to handle emergency stop
void handleEmergencyStop() {
    washingMachineServo.write(0); // Stop servo
    logToBT("Emergency Stop Activated! System Halted.");
    playMelody(stopMelody, stopNoteDurations, 5);
}

// Function to log messages
void logToBT(String message) {
    Serial.println(message);
    Serial1.println(message);
}

// Function to wait for an RFID card
void waitForCard() {
    if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
        String uid = "Card detected! UID: ";
        for (byte i = 0; i < rfid.uid.size; i++) {
            uid += String(rfid.uid.uidByte[i], HEX) + " ";
        }
        logToBT(uid);
        isCardAuthenticated = true;
        logToBT("Card authenticated. You can now use the system.");
        rfid.PICC_HaltA();
        rfid.PCD_StopCrypto1();
    }
}

// Function to handle user commands
void handleCommands() {
    if (Serial.available() > 0 || Serial1.available() > 0) {
        String command;
        if (Serial.available() > 0) {
            command = Serial.readStringUntil('\n');
        } else if (Serial1.available() > 0) {
            command = Serial1.readStringUntil('\n');
        }
        command.trim();

        if (command.equalsIgnoreCase("START")) {
            useMachine();
        } else {
            logToBT("Invalid command.");
        }
    }
}

// Function to start using the machine
void useMachine() {
    if (!authenticateCard()) return;
    logToBT("Starting washing machine...");
    playMelody(startMelody, startNoteDurations, 11);

    unsigned long startTime = millis();
    while (millis() - startTime < 10000) {
        if (emergencyStop) break; // Check emergency stop during operation
        washingMachineServo.write(90);
        delay(500);
        washingMachineServo.write(0);
        delay(500);
    }
    logToBT("Washing complete. Thank you!");
}

// Function to authenticate RFID card
bool authenticateCard() {
    if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) {
        logToBT("No card detected.");
        return false;
    }
    return true;
}

// Function to play melodies
void playMelody(int melody[], int noteDurations[], int length) {
    for (int i = 0; i < length; i++) {
        int noteDuration = noteDurations[i];
        if (melody[i] == 0) {
            delay(noteDuration); 
        } else {
            playTone(melody[i], noteDuration);
        }
    }
}

void playTone(int frequency, int duration) {
    int period = 1000000 / frequency;
    int pulse = period / 2;
    unsigned long endTime = millis() + duration;
    while (millis() < endTime) {
        digitalWrite(BUZZER_PIN, HIGH);
        delayMicroseconds(pulse);
        digitalWrite(BUZZER_PIN, LOW);
        delayMicroseconds(pulse);
    }
}
