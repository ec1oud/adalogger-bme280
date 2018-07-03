#include <Wire.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <SD.h>

#define VBATPIN A7

#define SEALEVELPRESSURE_HPA (1013.25)

const int chipSelect = 4;         // SPI chip select for SD card
const int cardDetect = 7;          // pin that detects whether the card is there
const int writeLed = 8;           // LED indicator for writing to card
const int errorLed = 13;          // LED indicator for error
const long interval = 9968;            // time between readings
const char fileName[] = "datalog.csv"; // filename to save on SD card
const char dataFieldNames[] = "Time,Battery(V),Temperature(*C),Humidity(%),Pressure(hPa)";

Adafruit_BME280 bme; // I2C
long lastWriteTime = 0;           // timestamp for last write attempt

float batteryVoltage() {
  float vbat = analogRead(VBATPIN);
  vbat *= 2;    // we divided by 2, so multiply back
  vbat *= 3.3;  // Multiply by 3.3V, our reference voltage
  vbat /= 1024; // convert to voltage
  return vbat;
}

boolean startSDCard() {
  // Wait until the card is inserted:
  while (digitalRead(cardDetect) == LOW) {
    Serial.println("Waiting for card...");
    digitalWrite(errorLed, HIGH);
    delay(100);
    digitalWrite(errorLed, LOW);
    delay(100);
    digitalWrite(errorLed, HIGH);
    delay(100);
    digitalWrite(errorLed, LOW);
    delay(700);
  }

  // wait until the card initialized successfully:
  while (!SD.begin(chipSelect)) {
    digitalWrite(errorLed, HIGH);   // turn on error LED
    Serial.println("Card failed");
    delay(750);
  }
  return true;
}

void setup() {
  pinMode(writeLed, OUTPUT);
  pinMode(errorLed, OUTPUT);
  pinMode(cardDetect, INPUT_PULLUP);

  if (!bme.begin(0x76)) {
      Serial.println("Could not find a valid BME280 sensor, check wiring!");
      while (1) {
          digitalWrite(errorLed, HIGH);
          delay(100);
          digitalWrite(errorLed, LOW);
          delay(100);
      }
  }

  // startSDCard() blocks everything until the card is present and writable:
  if (startSDCard() == true) {
    Serial.println("card initialized.");
    delay(100);
    // open the log file:
    File logFile = SD.open(fileName, FILE_WRITE);
    // write header columns to file:
    if (logFile) {
      logFile.println(dataFieldNames);
      logFile.close();
    }
  }

  Serial.println(dataFieldNames);
}

int appendCommaFloat(char *s, int len, int offset, float v) {
  return offset + snprintf(s + offset, len - offset, ",%d.%02d", int(v), int(v * 100.0F) % 100);
}

void loop() {
  // if the SD card is not there, don't do anything more:
  if (digitalRead(cardDetect) == LOW) {
    digitalWrite(errorLed, HIGH);
    return;
  }
  // turn off the error LED:
  digitalWrite(errorLed, LOW);

  if (millis()  - lastWriteTime >=  interval * 100)
    Serial.println(dataFieldNames);

  // read sensors every 10 seconds
  if (millis()  - lastWriteTime >=  interval) {
    char logLine[256];
    int offset = snprintf(logLine, sizeof(logLine), "%d", millis());
    offset = appendCommaFloat(logLine, sizeof(logLine), offset, batteryVoltage());
    offset = appendCommaFloat(logLine, sizeof(logLine), offset, bme.readTemperature());
    offset = appendCommaFloat(logLine, sizeof(logLine), offset, bme.readHumidity());
    offset = appendCommaFloat(logLine, sizeof(logLine), offset, bme.readPressure() / 100.0F);

    Serial.println(logLine);

    File logFile = SD.open(fileName, FILE_WRITE);   // open the log file
    if (logFile) {                                  // if you can write to the log file,
      digitalWrite(writeLed, HIGH);                 // turn on the write LED
      logFile.println(logLine);                       // write a line of data
      logFile.close();                              // close the file
      lastWriteTime = millis();
    }
    delay(100);
    digitalWrite(writeLed, LOW);                    // turn off the write LED
  }
}

