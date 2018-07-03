#include <Wire.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <SD.h>

#define VBATPIN A7

#define SEALEVELPRESSURE_HPA (1013.25)

const int chipSelect = 4;               // SPI chip select for SD card
const int cardDetect = 7;               // pin that detects whether the card is there
const int writeLed = 8;                 // LED indicator for writing to card
const int errorLed = 13;                // LED indicator for error
const long readInterval = 5000;         // time between readings
const long logInterval = 60000;         // time between writings
const char fileName[] = "datalog.csv";  // filename to save on SD card
const char dataFieldNames[] = "Time,Battery(V),Temperature(°C),Humidity(%),Pressure(hPa)";

Adafruit_BME280 bme; // I2C
long lastReadTime = 0;                  // timestamp for last sensor readings
long lastWriteTime = 0;                 // timestamp for last write to log file
bool sdInited = false;

float batteryVoltage() {
  float vbat = analogRead(VBATPIN);
  vbat *= 2;    // we divided by 2, so multiply back
  vbat *= 3.3;  // Multiply by 3.3V, our reference voltage
  vbat /= 1024; // convert to voltage
  return vbat;
}

void blinkError(int times) {
  for (int i = 0; i < times; ++i) {
    digitalWrite(errorLed, HIGH);
    delay(100);
    digitalWrite(errorLed, LOW);
    delay(100);
  }
}

bool startSDCard() {
  // card is inserted?
  if (digitalRead(cardDetect) == LOW) {
    Serial.println("SD card not detected");
    blinkError(2);
    return false;
  }

  // card is initialized successfully?
  if (!SD.begin(chipSelect)) {
    Serial.println("SD card failed");
    blinkError(3);
    return false;
  }
  return true;
}

void setup() {
  pinMode(writeLed, OUTPUT);
  pinMode(errorLed, OUTPUT);
  pinMode(cardDetect, INPUT_PULLUP);

  digitalWrite(errorLed, LOW);
  digitalWrite(writeLed, LOW);

  if (!bme.begin(0x76)) {
      Serial.println("Could not find a valid BME280 sensor, check wiring!");
      while (1) {
          blinkError(4);
          delay(1000);
      }
  }
}

int appendCommaFloat(char *s, int len, int offset, float v) {
  return offset + snprintf(s + offset, len - offset, ",%d.%02d", int(v), int(v * 100.0F) % 100);
}

void loop() {
  // read sensors and write the log file every 'interval' milliseconds
  if (!lastReadTime || millis() - lastReadTime >= readInterval) {
    char logLine[256];
    digitalWrite(writeLed, HIGH);
    int offset = snprintf(logLine, sizeof(logLine), "%d", millis());
    offset = appendCommaFloat(logLine, sizeof(logLine), offset, batteryVoltage());
    offset = appendCommaFloat(logLine, sizeof(logLine), offset, bme.readTemperature());
    offset = appendCommaFloat(logLine, sizeof(logLine), offset, bme.readHumidity());
    offset = appendCommaFloat(logLine, sizeof(logLine), offset, bme.readPressure() / 100.0F);
    if (!lastReadTime || millis() - lastReadTime >= logInterval)
      Serial.println(dataFieldNames);
    Serial.println(logLine);
    delay(5);
    digitalWrite(writeLed, LOW);
    lastReadTime = millis();

    bool sdWasInited = sdInited;
    if (!sdInited) 
      sdInited = startSDCard();

    if (digitalRead(cardDetect) == LOW) {
      if (sdWasInited) {
        Serial.println("SD card removed");
        blinkError(2);
      }
      sdInited = false;
      SD.end();
    }

    if (sdInited && (!sdWasInited || !lastWriteTime || millis() - lastWriteTime >= logInterval)) {
      File logFile = SD.open(fileName, FILE_WRITE);   // open the log file
      if (logFile) {                                  // if you can write to the log file,
        digitalWrite(writeLed, HIGH);                 // turn on the write LED
        if (!sdWasInited) {
          logFile.println(dataFieldNames);
          Serial.println("SD card found, log file started");
        }
        logFile.println(logLine);                     // write a line of data
        logFile.close();                              // close the file
        lastWriteTime = millis();
      }
      delay(500);
      digitalWrite(writeLed, LOW);                    // turn off the write LED
      Serial.println(dataFieldNames);
      delay((millis() + readInterval) % readInterval);
    }
  }
}

