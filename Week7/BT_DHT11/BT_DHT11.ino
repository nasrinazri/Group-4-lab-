#include <SoftwareSerial.h>
#include <DHT.h>

// Define RX and TX pins for SoftwareSerial
SoftwareSerial BTSerial(2, 3); // RX | TX

// DHT11 settings
#define DHTPIN 7      // Pin connected to DHT11 data pin
#define DHTTYPE DHT11 // Define the type of DHT sensor
DHT dht(DHTPIN, DHTTYPE);

void setup() {
  // Initialize Serial Monitor
  Serial.begin(9600);
  // Initialize Bluetooth Serial
  BTSerial.begin(9600);
  // Initialize DHT11 sensor
  dht.begin();

  Serial.println("DHT11 Bluetooth Data Transmission Starting...");
  BTSerial.println("Ready to display DHT11 data!");
}

void loop() {
  // Read temperature and humidity
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();

  // Check if the readings are valid
  if (isnan(humidity) || isnan(temperature)) {
    Serial.println("Failed to read from DHT sensor!");
    BTSerial.println("Error reading DHT sensor.");
    return;
  }

  // Display the data on the Serial Monitor
  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.println(" °C");
  Serial.print("Humidity: ");
  Serial.print(humidity);
  Serial.println(" %");

  // Send the data to the Bluetooth terminal
  BTSerial.print("Temperature: ");
  BTSerial.print(temperature);
  BTSerial.println(" °C");
  BTSerial.print("Humidity: ");
  BTSerial.print(humidity);
  BTSerial.println(" %");

  // Wait before the next reading
  delay(2000);
}