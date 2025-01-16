  #include <SPI.h>
  #include <MFRC522.h>
  #include <Servo.h>
  #include <Wire.h>
  #include <Keypad.h>
  #include <LiquidCrystal_I2C.h>

  Servo washingMachineServo;

  #define SLAVE_ADDRESS 0x08

  // Pin definitions
  #define RST_PIN 47 // Reset pin for RFID module
  #define SS_PIN 53  // Slave select pin for RFID module
  #define WASH_MACHINE_PIN 7 // Pin to control washing machine
  #define SLAVE_ADDRESS 0x08 // Define the slave device I²C address
  #define BT_BAUD_RATE 9600  // Baud rate for Bluetooth communication
  const int STOP_BUTTON_PIN = 2;  // Digital pin connected to the stop button

  #define GREEN_LED_PIN 26  // Green LED for "Ready"
  #define YELLOW_LED_PIN 24 // Yellow LED for "Washing"
  #define RED_LED_PIN 22   // Red LED for "Emergency Stop"

  volatile bool stopFlag = false; // Global flag to detect button press


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


    // Keypad setup
  const byte ROWS = 4; // Four rows
  const byte COLS = 4; // Four columns
  char keys[ROWS][COLS] = {
      {'1', '2', '3', 'A'},
      {'4', '5', '6', 'B'},
      {'7', '8', '9', 'C'},
      {'*', '0', '#', 'D'}
  };
  byte rowPins[ROWS] = {33, 31, 29, 27}; // Connect to R1, R2, R3, R4
  byte colPins[COLS] =  {41, 39, 37, 35}; // Connect to C1, C2, C3, C4

  Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);


  void setup() {
      pinMode(buzzerPin, OUTPUT); 
      pinMode(STOP_BUTTON_PIN, INPUT_PULLUP); // Enable internal pull-up resistor
      attachInterrupt(digitalPinToInterrupt(STOP_BUTTON_PIN), stopButtonISR, FALLING); // Interrupt on button press

      pinMode(GREEN_LED_PIN, OUTPUT);
      pinMode(YELLOW_LED_PIN, OUTPUT);
      pinMode(RED_LED_PIN, OUTPUT);

      digitalWrite(GREEN_LED_PIN, HIGH);  // Indicate system is ready at startup
      digitalWrite(YELLOW_LED_PIN, LOW);
      digitalWrite(RED_LED_PIN, LOW);

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
    char key = keypad.getKey();

    if (key) {
        handleInput(String(key));
    }

    if (!isCardAuthenticated) {
        indicateSystemReady();
        waitForCard();
    } else {
        if (Serial.available() > 0 || Serial1.available() > 0) {
            String command;
            if (Serial.available() > 0) {
                command = Serial.readStringUntil('\n');
            } else if (Serial1.available() > 0) {
                command = Serial1.readStringUntil('\n');
            }
            command.trim();
            handleInput(command);
        }
    }
}

  
  void stopButtonISR() {
      static unsigned long lastInterruptTime = 0;
      unsigned long interruptTime = millis();

      // Debounce check (ignore if within 200ms of the last interrupt)
      if (interruptTime - lastInterruptTime > 200) {
          stopFlag = true;  
          indicateEmergencyStop();  // Turn on red LED
          logToBT("Emergency stop activated.");// Set flag to stop the machine
      }
      lastInterruptTime = interruptTime;
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
      logToBT("4. Maintanence Mode (FIX): ");

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

    int currentTokens = buffer[0];
    logToBT("Current tokens: " + String(currentTokens));

    // Ask the user to input the token amount
    logToBT("Enter the number of tokens to add:");
    int tokensToAdd = getInputFromUser();

    if (tokensToAdd <= 0) {
        logToBT("Invalid token amount. Please enter a positive number.");
        return;
    }

    // Update the token balance
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

  void useMachine() {
      stopFlag = false;

      if (!authenticateCard()) return;

      byte blockAddr = 1;
      byte buffer[18];
      byte bufferSize = sizeof(buffer);

      if (rfid.MIFARE_Read(blockAddr, buffer, &bufferSize) != MFRC522::STATUS_OK) {
          logToBT("Reading failed.");
          rfid.PICC_HaltA();
          return;
      }

      indicateWashingState();

      int tokens = buffer[0];
      logToBT("Tokens available: " + String(tokens));

      // 🧼 Step 1: Ask user to choose the washing temperature
      logToBT("Select washing temperature:");
      logToBT("1. Hot (3 tokens)");
      logToBT("2. Warm (2 tokens)");
      logToBT("3. Cold (1 token)");

      int tempChoice = getInputFromUser();
      int tempTokens = 0;

      switch (tempChoice) {
          case 1:
              logToBT("You selected: Hot");
              tempTokens = 3;
              break;
          case 2:
              logToBT("You selected: Warm");
              tempTokens = 2;
              break;
          case 3:
              logToBT("You selected: Cold");
              tempTokens = 1;
              break;
          default:
              logToBT("Invalid choice. Please try again.");
              return;
      }

      if (tokens < tempTokens) {
          logToBT("Not enough tokens for the selected temperature. Required: " + String(tempTokens) + ", Available: " + String(tokens));
          return;
      }

      // Deduct tokens for temperature selection
      tokens -= tempTokens;
      logToBT("Tokens deducted for temperature: " + String(tempTokens) + ". Remaining tokens: " + String(tokens));

      // 🕒 Step 2: Ask user to enter washing time
      logToBT("Enter washing time in seconds (min: 10, increments of 10): ");
      int washingTime = getInputFromUser();

      if (washingTime < 10 || washingTime % 10 != 0) {
          logToBT("Invalid washing time. Please enter a multiple of 10 seconds.");
          return;
      }

      int requiredTokens = 1 + ((washingTime - 10) / 10);
      logToBT("Tokens required for time: " + String(requiredTokens));

      if (tokens >= requiredTokens) {
          tokens -= requiredTokens;
          buffer[0] = tokens;

          if (rfid.MIFARE_Write(blockAddr, buffer, 16) == MFRC522::STATUS_OK) {
              logToBT("Tokens deducted. Remaining tokens: " + String(tokens));
              logToBT("Starting washing machine for " + String(washingTime) + " seconds...");

              playMelody(startMelody, startNoteDurations, 11);

              unsigned long startTime = millis();
              unsigned long remainingTime = washingTime * 1000;

              while (millis() - startTime < remainingTime) {
                  if (stopFlag) {
                      logToBT("Operation stopped by user.");
                      break;
                  }
                  washingMachineServo.write(90);
                  delay(500);
                  washingMachineServo.write(0);
                  delay(500);
              }

              if (!stopFlag) {
                  indicateSystemReady();
                  logToBT("Washing complete. Thank you!");
              }

              printCommands();
          } else {
              logToBT("Failed to update token count.");
          }
      } else {
          logToBT("Not enough tokens. Required: " + String(requiredTokens) + ", Available: " + String(tokens));
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

        // Check Keypad input
        char key = keypad.getKey();
        if (key && isdigit(key)) {
            input += key;
            logToBT("Keypad input: " + String(key));
        } else if (key == '#') { // '#' to submit
            break;
        }
    }

    int value = input.toInt(); // Convert input to integer
    return value > 0 ? value : -1; // Return -1 if invalid input
}


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

  void indicateSystemReady() {
      digitalWrite(GREEN_LED_PIN, HIGH);
      digitalWrite(YELLOW_LED_PIN, LOW);
      digitalWrite(RED_LED_PIN, LOW);
  }

  void indicateWashingState() {
      digitalWrite(GREEN_LED_PIN, LOW);
      digitalWrite(YELLOW_LED_PIN, HIGH);
      digitalWrite(RED_LED_PIN, LOW);
  }

  void indicateEmergencyStop() {
      digitalWrite(GREEN_LED_PIN, LOW);
      digitalWrite(YELLOW_LED_PIN, LOW);
      digitalWrite(RED_LED_PIN, HIGH);

      delay(10000);

      indicateSystemReady();
  }

  // Function to request temperature from the slave
  void fix() {
      Wire.beginTransmission(8); // Start communication with slave at address 8
      Wire.write('F'); // Send the 'Fix' command to the slave
      Wire.endTransmission();

      delay(100); // Wait for slave to prepare the data

      Wire.requestFrom(8, 2); // Request 2 bytes for temperature data
      if (Wire.available() == 2) {
          int highByte = Wire.read(); // Read the high byte
          int lowByte = Wire.read();  // Read the low byte
          float temperature = (highByte << 8 | lowByte) / 100.0; // Combine bytes and scale back
          Serial.print("Temperature received from slave: ");
          Serial.print(temperature);
          Serial.println(" °C");
          logToBT("Current temperature: " + String(temperature) + " °C");
      } else {
          Serial.println("Error: Could not retrieve temperature from slave.");
      }

      printCommands();
  }
