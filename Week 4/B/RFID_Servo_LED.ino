#include <SPI.h>
#include <MFRC522.h>
#include <Servo.h>

// Pin definitions
#define RST_PIN         49           // Reset pin for RFID
#define SS_PIN          53          // SS/SDA pin for RFID
#define SERVO_PIN       3           // Servo control pin
#define GREEN_LED_PIN   4           // Green LED
#define RED_LED_PIN     5           // Red LED

// Instances
MFRC522 rfid(SS_PIN, RST_PIN);     // Create MFRC522 instance
Servo accessServo;                  // Create servo instance

// Array of authorized cards
byte authorizedCards[][4] = {
  {0x33, 0x36, 0x87, 0x1A} // Card for granted and tag for decline (Not Here Easier to code then making another function)
};

const int NUM_CARDS = sizeof(authorizedCards) / sizeof(authorizedCards[0]);

void setup() {
  Serial.begin(9600);               
  SPI.begin();                      // Initialize SPI bus
  rfid.PCD_Init();                  // Initialize RFID reader
  
  // Initialize servo
  accessServo.attach(SERVO_PIN);
  accessServo.write(0);             // Set initial position
  
  // Initialize LEDs
  pinMode(GREEN_LED_PIN, OUTPUT);
  pinMode(RED_LED_PIN, OUTPUT);
  
  Serial.println("System Ready");
}

void loop() {
  // Reset the loop if no new card is present
  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial())
    return;

  // Check if card is authorized
  if (isAuthorizedCard(rfid.uid.uidByte)) {
    grantAccess();
  } else {
    denyAccess();
  }

  // Halt PICC and stop encryption
  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
}

bool isAuthorizedCard(byte *cardUID) {
  for (int i = 0; i < NUM_CARDS; i++) {
    if (compareUIDs(cardUID, authorizedCards[i])) {
      return true;
    }
  }
  return false;
}

bool compareUIDs(byte *uid1, byte *uid2) { //comparison algorithm
  for (int i = 0; i < 4; i++) {
    if (uid1[i] != uid2[i]) {
      return false;
    }
  }
  return true;
}

void grantAccess() {
  Serial.println("Access granted");
  digitalWrite(GREEN_LED_PIN, HIGH);
  digitalWrite(RED_LED_PIN, LOW);
  
  // Move servo to open position
  accessServo.write(90);
  delay(1000);                      // Keep open for 1 seconds
  
  // Return to closed position
  accessServo.write(0);
  digitalWrite(GREEN_LED_PIN, LOW);
}

void denyAccess() {
  Serial.println("Access denied");
  digitalWrite(RED_LED_PIN, HIGH);
  delay(1000);
  digitalWrite(RED_LED_PIN, LOW);
}
