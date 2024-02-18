#include <Servo.h>
#include <Wire.h>
#include <Adafruit_INA219.h>
#include <SPI.h>
#include <SD.h>

// Define pin numbers for each component
const int RELAY_1_PIN = 8;
const int RELAY_2_PIN = 7;
const int INA219_SDA_PIN = A4;
const int INA219_SCL_PIN = A5;
const int SD_CS_PIN = 4;
const int SD_SCK_PIN = 13;
const int SD_MOSI_PIN = 11;
const int SD_MISO_PIN = 12;

Servo BOOSTER_Servo;
Servo Horizontal_Servo;
Servo Vertical_Servo;

Adafruit_INA219 ina219;

int ldr_top_right = A1;
int ldr_top_left = A0;
int ldr_bottom_right = A2;
int ldr_bottom_left = A3;

const int SENSOR_PIN = A0;
File dataFile;

void setup() {
  Serial.begin(9600);
  pinMode(RELAY_1_PIN, OUTPUT);
  pinMode(RELAY_2_PIN, OUTPUT);
  BOOSTER_Servo.attach(3);
  Vertical_Servo.attach(5);
  Horizontal_Servo.attach(9);

  // Set initial angle for BOOSTER_Servo
  BOOSTER_Servo.write(180);
  delay(1000); // Delay to let the servo reach its initial position

  // Move servo from 180 degrees to 60 degrees
  for (int angle = 180; angle >= 60; angle--) {
    BOOSTER_Servo.write(angle);
    delay(15);
  }
  delay(1000); // Delay to keep the servo at 60 degrees for a second

  // Return servo to 180 degrees
  for (int angle = 60; angle <= 180; angle++) {
    BOOSTER_Servo.write(angle);
    delay(15);
  }

  // Initialize INA219
  if (!ina219.begin()) {
    Serial.println("Failed to find INA219 chip");
    while (1) {
      delay(10);
    }
  }

  // Initialize SD card
  if (!SD.begin(SD_CS_PIN)) {
    Serial.println("SD card initialization failed!");
    while (1);
  }
  Serial.println("SD card initialized successfully!");

  // Create or open data file
  dataFile = SD.open("data.csv", FILE_WRITE);

  if (!dataFile) {
    Serial.println("Error opening data file!");
    while (1);
  }

  // Write headers to the file
  dataFile.print("Time, ");
  dataFile.print("Bus Voltage (V), ");
  dataFile.print("Shunt Voltage (mV), ");
  dataFile.print("Load Voltage (V), ");
  dataFile.print("Current (mA), ");
  dataFile.print("Power (mW), ");
  dataFile.print("LDR Top Right, ");
  dataFile.print("LDR Top Left, ");
  dataFile.print("LDR Bottom Right, ");
  dataFile.println("LDR Bottom Left");

  Serial.println("Logging sensor data...");
  
  // Initially, set relay 2 high and relay 1 low
  digitalWrite(RELAY_2_PIN, HIGH);
  digitalWrite(RELAY_1_PIN, LOW);
  delay(1000); // Delay for 1 second
  digitalWrite(RELAY_2_PIN, LOW);
  digitalWrite(RELAY_1_PIN, HIGH);
}
void loop() {
  float shuntvoltage = 0;
  float busvoltage = 0;
  float current_mA = 0;
  float loadvoltage = 0;
  float power_mW = 0;

  shuntvoltage = ina219.getShuntVoltage_mV();
  busvoltage = ina219.getBusVoltage_V();
  current_mA = ina219.getCurrent_mA();
  power_mW = ina219.getPower_mW();
  loadvoltage = busvoltage + (shuntvoltage / 1000);

  int top_right_value = analogRead(ldr_top_right);
  int top_left_value = analogRead(ldr_top_left);
  int bottom_right_value = analogRead(ldr_bottom_right);
  int bottom_left_value = analogRead(ldr_bottom_left);

  unsigned long currentTime = millis();

  Serial.print("Bus Voltage:   "); Serial.print(busvoltage); Serial.println(" V");
  Serial.print("Shunt Voltage: "); Serial.print(shuntvoltage); Serial.println(" mV");
  Serial.print("Load Voltage:  "); Serial.print(loadvoltage); Serial.println(" V");
  Serial.print("Current:       "); Serial.print(current_mA); Serial.println(" mA");
  Serial.print("Power:         "); Serial.print(power_mW); Serial.println(" mW");
  Serial.print("LDR Top Right: "); Serial.println(top_right_value);
  Serial.print("LDR Top Left:  "); Serial.println(top_left_value);
  Serial.print("LDR Bottom Right: "); Serial.println(bottom_right_value);
  Serial.print("LDR Bottom Left:  "); Serial.println(bottom_left_value);
  Serial.println("");

  // Write data to the file
  dataFile.print(currentTime); dataFile.print(", ");
  dataFile.print(busvoltage); dataFile.print(", ");
  dataFile.print(shuntvoltage); dataFile.print(", ");
  dataFile.print(loadvoltage); dataFile.print(", ");
  dataFile.print(current_mA); dataFile.print(", ");
  dataFile.print(power_mW); dataFile.print(", ");
  dataFile.print(top_right_value); dataFile.print(", ");
  dataFile.print(top_left_value); dataFile.print(", ");
  dataFile.print(bottom_right_value); dataFile.print(", ");
  dataFile.println(bottom_left_value);

  // Flush the data to the card
  dataFile.flush();

  // Step 1
  if (busvoltage < 5 && power_mW > 150) {
    digitalWrite(RELAY_2_PIN, HIGH); // Turn on relay 2
  } else if (busvoltage > 5) {
    digitalWrite(RELAY_2_PIN, LOW);  // Turn off relay 2
    digitalWrite(RELAY_1_PIN, HIGH); // Turn on relay 1
    delay(1000);                      // Delay for 1 second
    digitalWrite(RELAY_1_PIN, LOW);  // Turn off relay 1
  }

  // Step 2
  if (power_mW < 150) {
    BOOSTER_Servo.write(100); // Set servo to 100 degrees
  } else {
    // Calculate the difference between opposite LDR pairs
    int horizontal_diff = (top_left_value + bottom_left_value) - (top_right_value + bottom_right_value);
    int vertical_diff = (top_left_value + top_right_value) - (bottom_left_value + bottom_right_value);

    // Map the difference to the servo angle range (-90 to 90 degrees)
    int horizontal_angle = map(horizontal_diff, -1023, 1023, -90, 90);
    int vertical_angle = map(vertical_diff, -1023, 1023, -90, 90);

    // Apply the angles to the servos
    Horizontal_Servo.write(horizontal_angle);
    Vertical_Servo.write(vertical_angle);
    delay(1000); // Delay for 1 second
  }
}
