// System 2: Washing Machine Cloth Checker

// Arduino IDE 2.3.4

#include <Pixy.h>              // For the Pixy camera module to detect objects and colors
#include <LiquidCrystal_I2C.h> // For the I2C LCD display
#include <NewPing.h>           // For the ultrasonic distance sensor
#include <SPI.h>               // Required for Pixy camera communication

// Define pins and constants for the system
#define TRIGGER_PIN  22        // Ultrasonic sensor trigger pin
#define ECHO_PIN     23        // Ultrasonic sensor echo pin
#define LED_PIN      13        // LED indicator for object presence detection
#define WHITE_LED_PIN 12       // LED indicator specifically for white shirt detection
#define MAX_DISTANCE 200       // Maximum detection distance in centimeters
#define MIN_BLOCK_SIZE 20      // Minimum size of detected object to be considered valid
#define SCAN_ATTEMPTS 3        // Number of attempts to scan for white shirt

// Initialize hardware objects
Pixy pixy;                     // Create Pixy camera object
LiquidCrystal_I2C lcd(0x27, 16, 2);  // Initialize LCD with address 0x27, 16 columns, 2 rows
NewPing sonar(TRIGGER_PIN, ECHO_PIN, MAX_DISTANCE);  // Initialize ultrasonic sensor

// System state variables
bool clothDetected = false;    // Tracks if any object is currently detected
int whiteShirtSignature = 1;   // Pixy camera signature ID for white shirt
unsigned long lastDetectionTime = 0;  // Timestamp of last detection
const unsigned long DETECTION_TIMEOUT = 1000;  // Time before allowing new detection (debouncing)
bool isWhiteShirtDetected = false;    // Tracks if a white shirt is specifically detected

void setup() {
  Serial.begin(9600);          // Initialize serial communication for debugging
  
  // Configure LED pins
  pinMode(LED_PIN, OUTPUT);
  pinMode(WHITE_LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  digitalWrite(WHITE_LED_PIN, LOW);
  
  // Initialize LCD display
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.print("System Ready");
  
  pixy.init();                 // Initialize Pixy camera
  Serial.println("Initialization complete");
}

void loop() {
  // Get distance reading from ultrasonic sensor
  int distance = sonar.ping_cm();
  
  // Check if object is within detection range (0-20cm)
  if (distance > 0 && distance < 20) {
    // Only process if no recent detection or timeout has passed (debouncing)
    if (!clothDetected || (millis() - lastDetectionTime > DETECTION_TIMEOUT)) {
      clothDetected = true;
      lastDetectionTime = millis();
      digitalWrite(LED_PIN, HIGH);  // Turn on presence detection LED
      
      // Scan for white shirt and update detection status
      isWhiteShirtDetected = scanForWhiteShirt();
      
      // Control white shirt indicator LED based on detection
      digitalWrite(WHITE_LED_PIN, isWhiteShirtDetected ? HIGH : LOW);
      
      // Update LCD display with detection results
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
    // Reset system state when no object is detected
    clothDetected = false;
    isWhiteShirtDetected = false;
    digitalWrite(LED_PIN, LOW);
    digitalWrite(WHITE_LED_PIN, LOW);
    lcd.clear();
    lcd.print("System Ready");
  }
  
  delay(50);  // Small delay to prevent excessive processing
}

// Function to detect white shirt using Pixy camera
bool scanForWhiteShirt() {
  int successfulScans = 0;     // Counter for successful white shirt detections
  
  // Perform multiple scan attempts for reliable detection
  for (int attempt = 0; attempt < SCAN_ATTEMPTS; attempt++) {
    uint16_t blocks = pixy.getBlocks();  // Get detected objects from Pixy
    
    if (blocks) {
      // Check each detected block
      for (uint16_t i = 0; i < blocks; i++) {
        // Verify if block matches white shirt criteria (signature and size)
        if (pixy.blocks[i].signature == whiteShirtSignature && 
            pixy.blocks[i].width > MIN_BLOCK_SIZE && 
            pixy.blocks[i].height > MIN_BLOCK_SIZE) {
          successfulScans++;
          
          // Log detection details for debugging
          Serial.print("Block detected - Width: ");
          Serial.print(pixy.blocks[i].width);
          Serial.print(" Height: ");
          Serial.println(pixy.blocks[i].height);
          
          break;  // Exit current scan if white shirt is detected
        }
      }
    }
    delay(50);  // Brief delay between scan attempts
  }
  
  // Determine final detection result (majority of scans must be successful)
  bool detected = successfulScans > SCAN_ATTEMPTS/2;
  
  // Log final detection results
  Serial.print("Detection result: ");
  Serial.println(detected ? "White shirt detected" : "No white shirt found");
  Serial.print("Successful scans: ");
  Serial.println(successfulScans);
  
  return detected;
}
