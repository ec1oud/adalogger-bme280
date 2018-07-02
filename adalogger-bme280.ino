#include <Wire.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

#define VBATPIN A7

#define SEALEVELPRESSURE_HPA (1013.25)

Adafruit_BME280 bme; // I2C

extern "C" char *sbrk(int i);
 
int FreeRam () {
  char stack_dummy = 0;
  return &stack_dummy - sbrk(0);
}
    
void checkBattery() {
  float measuredvbat = analogRead(VBATPIN);
  measuredvbat *= 2;    // we divided by 2, so multiply back
  measuredvbat *= 3.3;  // Multiply by 3.3V, our reference voltage
  measuredvbat /= 1024; // convert to voltage
  Serial.print("VBat: " ); Serial.println(measuredvbat);
}

void printValues() {
    Serial.print("Temperature = ");
    Serial.print(bme.readTemperature());
    Serial.println(" *C");

    Serial.print("Pressure = ");

    Serial.print(bme.readPressure() / 100.0F);
    Serial.println(" hPa");

    Serial.print("Approx. Altitude = ");
    Serial.print(bme.readAltitude(SEALEVELPRESSURE_HPA));
    Serial.println(" m");

    Serial.print("Humidity = ");
    Serial.print(bme.readHumidity());
    Serial.println(" %");

    Serial.println();
}

void setup() {
  // put your setup code here, to run once:
  pinMode(13, OUTPUT);
  
  
  bool status;
  
  // default settings
  // (you can also pass in a Wire library object like &Wire2)
  status = bme.begin(0x76);  
  if (!status) {
      Serial.println("Could not find a valid BME280 sensor, check wiring!");
      while (1) {
          digitalWrite(13, HIGH);  
          delay(100);              
          digitalWrite(13, LOW);   
          delay(100);             
      }
  }
}

void loop() {
  // put your main code here, to run repeatedly:
  digitalWrite(13, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(1000);              // wait for a second
  digitalWrite(13, LOW);    // turn the LED off by making the voltage LOW
  delay(1000);              // wait for a second
  checkBattery();
  Serial.print("Mem: " ); Serial.println(FreeRam());
  printValues();
}

