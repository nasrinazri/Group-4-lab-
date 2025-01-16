#include <Pixy.h>
Pixy pixy;
void setup() {
  Serial.begin(9600);
  pixy.init();
}

void loop() {
  int blocks = pixy.getBlocks();
  if (blocks) {
    // Loop through detected blocks
    for (int i = 0; i < blocks; i++) {
      Serial.print("Block ");
      Serial.print(i);
      Serial.print(": ");
      Serial.print("Signature: ");
      Serial.print(pixy.blocks[i].signature);
      Serial.print(" X: "); 
      Serial.print(pixy.blocks[i].x);
      Serial.print(" Y: ");
      Serial.println(pixy.blocks[i].y);
      // Add logic to identify and react based on the color signature
      if (pixy.blocks[i].signature == 1) {
      // Object with signature 1 detected (Color 1)
        digitalWrite(5,HIGH);

      } else if (pixy.blocks[i].signature == 2) {
      // Object with signature 2 detected (Color 2)
        digitalWrite(5,LOW);
   }
}

