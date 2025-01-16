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
  const int buzzerPin2 = 5;   // Pin connected to the buzzer
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

  // Define keypad matrix dimensions and pin connections
  const byte ROWS = 4;  // Number of rows
  const byte COLS = 4;  // Number of columns

  // Define keymap for the keypad
  char keys[ROWS][COLS] = {
      {'1', '2', '3', 'A'},
      {'4', '5', '6', 'B'},
      {'7', '8', '9', 'C'},
      {'*', '0', '#', 'D'}
  };

  // Define the row and column pin connections
  byte rowPins[ROWS] = {33, 31, 29, 27}; // Connect to R1, R2, R3, R4
  byte colPins[COLS] =  {41, 39, 37, 35}; // Connect to C1, C2, C3, C4

// Create Keypad object
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);



  void setup() {
      pinMode(buzzerPin, OUTPUT); 
      pinMode(buzzerPin2, OUTPUT); 
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
    if (!isCardAuthenticated) {
        indicateSystemReady();
        waitForCard();
    } else {
        char key = keypad.getKey(); // Check for input from the keypad

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
                  playMelody2(stopMelody, startNoteDurations, 5);

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

  void playTone2(int frequency, int duration) {
      int period = 1000000 / frequency; // Calculate the period in microseconds
      int pulse = period / 2;           // Calculate the pulse width
      unsigned long endTime = millis() + duration;
      while (millis() < endTime) {
          digitalWrite(buzzerPin2, HIGH);
          delayMicroseconds(pulse);
          digitalWrite(buzzerPin2, LOW);
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
    Wire.beginTransmission(8);    // Start communication with slave at address 8
    Wire.write('F');              // Send the 'Fix' command to the slave
    Wire.endTransmission();
    delay(100);                   // Wait for slave to prepare the data
    
    Wire.requestFrom(8, 6);       // Request 6 bytes (2 for temp, 2 for water, 2 for load cell)
    
    if (Wire.available() == 6) {
        // Read temperature bytes
        int highByteTemp = Wire.read();
        int lowByteTemp = Wire.read();
        float temperature = (highByteTemp << 8 | lowByteTemp) / 100.0;
        
        // Read water level bytes
        int highByteWater = Wire.read();
        int lowByteWater = Wire.read();
        int waterLevel = (highByteWater << 8 | lowByteWater);
        
        // Read load cell bytes
        int highByteLoad = Wire.read();
        int lowByteLoad = Wire.read();
        float loadCell = (highByteLoad << 8 | lowByteLoad) / 100.0;
        
        // Display all sensor values
        Serial.print("Temperature received from slave: ");
        Serial.print(temperature);
        Serial.println(" °C");
        
        Serial.print("Water level received from slave: ");
        Serial.println(waterLevel);
        
        Serial.print("Load cell value received from slave: ");
        Serial.print(loadCell);
        Serial.println(" units");
        
        logToBT("Current temperature: " + String(temperature) + " °C");
        logToBT("Current water level: " + String(waterLevel));
        logToBT("Current load cell reading: " + String(loadCell) + " units");
    } else {
        Serial.println("Error: Could not retrieve data from slave.");
    }
    printCommands();
}


  void processCommand(String command) {
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


  void processKeypadCommand(char key) {
    if (key == 'A') {  // Add tokens
        logToBT("Keypad input: A (Add Tokens)");
        logToBT("Enter the number of tokens to add:");
        int tokensToAdd = getNumericInputFromKeypad();  // Get numeric input from the keypad
        if (tokensToAdd > 0) {
            logToBT("Adding tokens: " + String(tokensToAdd));
            addTokensFromKeypad(tokensToAdd);
        } else {
            logToBT("Invalid input for tokens.");
        }

    } else if (key == 'B') {  // Check tokens
        logToBT("Keypad input: B (Check Tokens)");
        checkTokens();

    } else if (key == 'C') {  // Start the washing machine
        logToBT("Keypad input: C (Start Machine)");
        logToBT("Select washing temperature:");
        logToBT("1. Hot (3 tokens)");
        logToBT("2. Warm (2 tokens)");
        logToBT("3. Cold (1 token)");
        int tempChoice = getNumericInputFromKeypad();
        if (tempChoice < 1 || tempChoice > 3) {
            logToBT("Invalid temperature choice.");
            return;
        }

        logToBT("Enter washing time in seconds (min: 10, increments of 10):");
        int washingTime = getNumericInputFromKeypad();
        if (washingTime < 10 || washingTime % 10 != 0) {
            logToBT("Invalid washing time.");
            return;
        }

        logToBT("Starting machine with temperature choice: " + String(tempChoice) + " and time: " + String(washingTime));
        startMachineWithKeypad(tempChoice, washingTime);

    } else if (key == 'D') {  // Maintenance mode
        logToBT("Keypad input: D (Fix)");
        fix();

    } else {  // Invalid key
        logToBT("Invalid key pressed on keypad. Use A, B, C, or D.");
    }
}


  int getNumericInputFromKeypad() {
    String input = ""; // Buffer to store numeric input
    char key;

    logToBT("Enter a number:");

    // Wait for numeric input from keypad
    while (true) {
        key = keypad.getKey();
        if (key) {
            if (isdigit(key)) {
                input += key; // Append digit to input buffer
                logToBT("Entered: " + input); // Optional: Echo input to BT
            } else if (key == '#') { // '#' signals end of input
                break;
            } else {
                logToBT("Invalid key. Enter digits only, end with '#'.");
            }
        }
    }

    return input.toInt(); // Convert the input string to an integer
}


  void addTokensFromKeypad(int tokensToAdd) {
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

  void startMachineWithKeypad(int tempChoice, int washingTime) {
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

    int tokens = buffer[0];
    int tempTokens = (tempChoice == 1) ? 3 : (tempChoice == 2) ? 2 : 1;

    if (tokens < tempTokens) {
        logToBT("Not enough tokens for the selected temperature.");
        return;
    }

    int requiredTokens = 1 + ((washingTime - 10) / 10);
    if (tokens < tempTokens + requiredTokens) {
        logToBT("Not enough tokens for the selected time.");
        return;
    }

    tokens -= (tempTokens + requiredTokens);
    buffer[0] = tokens;

    if (rfid.MIFARE_Write(blockAddr, buffer, 16) == MFRC522::STATUS_OK) {
        logToBT("Tokens deducted. Remaining tokens: " + String(tokens));
        logToBT("Starting washing machine...");
        indicateWashingState();
        
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
            playMelody2(stopMelody, startNoteDurations, 5);
        }
    } else {
        logToBT("Failed to update token count.");
    }

    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
}



