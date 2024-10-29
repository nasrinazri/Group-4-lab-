void setup()
{
 Serial.begin(9600);
}
void loop()
{

 int potValue = analogRead(A0);
 Serial.println(potValue);
 delay(1000); // add a delay to avoid sending data too fast
}