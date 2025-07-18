/*
 * This code is to be uploaded to the base package or solar package. The EPM package uses EPM_data_collection.ino. 
 * This is the sketch that will run during deployment and take stage, temperature, pressure, humidity, and power
 * measurements. The data is saved on an SD card and collection occurs every five minutes as dictated by RTC alarms.
 * 
 * To change the data collection cycle interval: edit the const int time_interval
 * 
 * Written by Corinne Smith 
 * Date: Jan 2022
*/

#include <SD.h>                         // for SD card module
#include <SPI.h>                        // for SPI
#include <Wire.h>                       // for I2C
#include <HCSR04.h>                     // for the USS
#include <DS3232RTC.h>                  // for the RTC https://github.com/JChristensen/DS3232RTC
DS3232RTC RTC;
#include <avr/sleep.h>                  // for sleep mode
#include <Adafruit_Sensor.h>            // for BME 
#include <Adafruit_BME280.h>            // for BME
#include <Adafruit_INA219.h>            // for voltage monitor

const int RTCinterrupt = 2;             // RTC interrupt from sleep mode on digital pin 2
#define SEALEVELPRESSURE_HPA (1013.25)  // constant for bme


// HC-SR04 ----------------------------------------------------------------------------------------------
const int trigPin = 9;     
const int echoPin = 8;      
UltraSonicDistanceSensor distanceSensor(trigPin, echoPin);

// SD ---------------------------------------------------------------------------------------------------
const int pinCS = 10;

// BME280 -----------------------------------------------------------------------------------------------
Adafruit_BME280 bme;

// INA219 -----------------------------------------------------------------------------------------------
Adafruit_INA219 ina219;

// controls ---------------------------------------------------------------------------------------------
const int LED = A3;
const int time_interval = 5;          // interval in which the minutes will be delayed. Default is five minutes
unsigned long prevTimeElapsed = 0;

void setup() {
  Serial.begin(9600);
  pinMode(pinCS, OUTPUT);
  pinMode(LED, OUTPUT);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(RTCinterrupt, INPUT_PULLUP);            // configure the interrupt pin using the built-in pullup resistor
  Serial.println("Executing data_collection.ino");

  // BME initialization ------------------------------------------------------------------------------------------------------------------------------
  bme.begin(0x76);

  // Voltage regulator initialization -----------------------------------------------------------------------------------------------------------------
  uint32_t currentFrequency;
  ina219.begin();
}

void loop() {
  digitalWrite(LED, LOW);     // turn off LED before sleeping
  delay(1000);
  float busvoltage = ina219.getBusVoltage_V();
  float current_mA = ina219.getCurrent_mA();
  float power_mW = ina219.getPower_mW();

  Serial.print(busvoltage); Serial.print(" ");
  Serial.print(current_mA); Serial.print(" ");
  Serial.println(power_mW); 
}
