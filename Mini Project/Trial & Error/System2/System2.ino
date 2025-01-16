#include <Pixy.h>
#include <LiquidCrystal_I2C.h>
#include <NewPing.h>
#include <SPI.h>

// Pin definitions
#define TRIGGER_PIN  22
#define ECHO_PIN     23
#define LED_PIN      13     // LED for object detection
#define WHITE_LED_PIN 12    // New LED pin for white shirt detection
#define MAX_DISTANCE 200
#define MIN_BLOCK_SIZE 20
#define SCAN_ATTEMPTS 3

// Initialize objects
Pixy pixy;
LiquidCrystal_I2C lcd(0x27, 16, 2);
NewPing sonar(TRIGGER_PIN, ECHO_PIN, MAX_DISTANCE);

// Variables
bool clothDetected = false;
int whiteShirtSignature = 1;
unsigned long lastDetectionTime = 0;
const unsigned long DETECTION_TIMEOUT = 1000;
bool isWhiteShirtDetected = false;

void setup() {
  Serial.begin(9600);
  
  pinMode(LED_PIN, OUTPUT);
  pinMode(WHITE_LED_PIN, OUTPUT);  // Initialize the new LED pin
  digitalWrite(LED_PIN, LOW);
  digitalWrite(WHITE_LED_PIN, LOW);
  
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.print("System Ready");
  
  pixy.init();
  Serial.println("Initialization complete");
}

void loop() {
  int distance = sonar.ping_cm();
  
  if (distance > 0 && distance < 20) {
    if (!clothDetected || (millis() - lastDetectionTime > DETECTION_TIMEOUT)) {
      clothDetected = true;
      lastDetectionTime = millis();
      digitalWrite(LED_PIN, HIGH);
      
      // Check for white shirt without showing "Scanning..."
      isWhiteShirtDetected = scanForWhiteShirt();
      
      // Control white shirt LED
      digitalWrite(WHITE_LED_PIN, isWhiteShirtDetected ? HIGH : LOW);
      
      // Update display based on detection result
      lcd.clear();
      if (isWhiteShirtDetected) {
        lcd.print("White shirt");
        lcd.setCursor(0, 1);
        lcd.print("detected!");
      } else {
        lcd.print("No white shirt");
        lcd.setCursor(0, 1);
        lcd.print("found");
      }
    }
  } else {
    clothDetected = false;
    isWhiteShirtDetected = false;
    digitalWrite(LED_PIN, LOW);
    digitalWrite(WHITE_LED_PIN, LOW);  // Turn off white shirt LED
    lcd.clear();
    lcd.print("System Ready");
  }
  
  delay(50);
}

bool scanForWhiteShirt() {
  int successfulScans = 0;
  
  for (int attempt = 0; attempt < SCAN_ATTEMPTS; attempt++) {
    uint16_t blocks = pixy.getBlocks();
    
    if (blocks) {
      for (uint16_t i = 0; i < blocks; i++) {
        if (pixy.blocks[i].signature == whiteShirtSignature && 
            pixy.blocks[i].width > MIN_BLOCK_SIZE && 
            pixy.blocks[i].height > MIN_BLOCK_SIZE) {
          successfulScans++;
          
          Serial.print("Block detected - Width: ");
          Serial.print(pixy.blocks[i].width);
          Serial.print(" Height: ");
          Serial.println(pixy.blocks[i].height);
          
          break;
        }
      }
    }
    delay(50);
  }
  
  bool detected = successfulScans > SCAN_ATTEMPTS/2;
  Serial.print("Detection result: ");
  Serial.println(detected ? "White shirt detected" : "No white shirt found");
  Serial.print("Successful scans: ");
  Serial.println(successfulScans);
  
  return detected;
}