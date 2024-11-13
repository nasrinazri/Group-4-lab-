float tempcelc;
int lm_value; 
int ldr_value;
int ldr_percent;

void setup() {
  Serial.begin(9600);
  Serial.println("CLEARDATA");
  Serial.println("LABEL,CLOCK,TEMPERATURE,LIGHT"); // Labels for PLX-DAQ columns
}

void loop() {
  // Read temperature from LM35 sensor
  lm_value = analogRead(A0);
  //tempcelc = ((5 * lm_value* 100)/1024);
  tempcelc = (lm_value / 1023.0) * 5000; // Convert to millivolts
  tempcelc = tempcelc / 10; // Convert to degrees Celsius

  // Read LDR value and map it to a percentage
  ldr_value = analogRead(A1);
  ldr_percent = map(ldr_value, 0, 1023, 0, 100);

  // Print both temperature and light on the same line for PLX-DAQ
  Serial.print("DATA,TIME,");
  Serial.print(tempcelc); // Print temperature value
  Serial.print(",");      // Separator for next data
  Serial.println(ldr_percent); // Print light percentage value

  delay(1500); // Delay for 1.5 seconds
}
