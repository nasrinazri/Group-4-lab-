const int upStartPin = 2;    // NO contact
const int downStopPin = 3;   // NC contact
const int ledPin = 7;
bool latch = false;

void setup() {
  pinMode(upStartPin, INPUT_PULLUP);   // Using pullup for NO contact
  pinMode(downStopPin, INPUT_PULLUP);  // Using pullup for NC contact
  pinMode(ledPin, OUTPUT);
  Serial.begin(9600);  
}

void loop() {
  // For NO contact (Up_Start): LOW when pressed (TRUE)
  bool upStart = (digitalRead(upStartPin) == LOW);
  
  // For NC contact (Down_Stop): HIGH when not pressed (TRUE)
  bool downStop = (digitalRead(downStopPin) == HIGH);
  
  // Implementing the self-holding circuit:
  // Latch will be TRUE if:
  // 1. Up_Start is pressed OR
  // 2. Latch is already TRUE AND Down_Stop is not pressed
  latch = upStart || (latch && downStop);
  
  // Control LED based on latch state
  digitalWrite(ledPin, latch);
  
  // Debug information
  Serial.print("Up_Start: "); Serial.print(upStart);
  Serial.print(" Down_Stop: "); Serial.print(downStop);
  Serial.print(" Latch: "); Serial.println(latch);
  delay(100);
}