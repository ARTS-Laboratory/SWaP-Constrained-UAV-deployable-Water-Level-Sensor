/*
 * This code is to be uploaded to the base package or solar package. The EPM package uses EPM_data_collection.ino. 
 * This is the sketch that will run during deployment and take stage, temperature, pressure, humidity, and power
 * measurements. The data is saved on an SD card and collection occurs every five minutes as dictated by RTC alarms.
 * 
 * Written by Corinne Smith
 * July 9 2021
 * Modified from ArduinoGetStarted, www.ArduinoGetStarted.com
 * Modified from Dejan Nedelkowski, www.HowToMechatronics.com
 * Modified from ArduinoLearning, www.ArduinoLearning.com
*/

#include <SD.h>                         // for SD card module
#include <SPI.h>                        // for SPI
#include <Wire.h>                       // for I2C
#include <HCSR04.h>                     // for the USS
#include <DS3232RTC.h>                  // for the RTC https://github.com/JChristensen/DS3232RTC
#include <nRF24L01.h>                   // for the nRF24L01
#include <RF24.h>                       // for the nRF24L01
#include <avr/sleep.h>                  // for sleep mode
#include <Adafruit_Sensor.h>            // for BME 
#include <Adafruit_BME280.h>            // for BME
#include <Adafruit_INA219.h>            // for voltage monitor

const int RTCinterrupt = 2;             // RTC interrupt from sleep mode on digital pin 2 (INT_0)
const int nRFinterrupt = 3;             // nRF interrupt from sleep mode on digital pin 3 (INT_1)
#define SEALEVELPRESSURE_HPA (1013.25)  // constant for bme

// nRF24L01 -------------------------------------------------------------------------------------------
RF24 radio(7, 6); // CE, CSN
const byte addresses [][6] = {"00001", "00002", "00003", "00004"};  //Setting the two addresses. One for transmitting and one for receiving
int command = 0;      
int PC = 4;
int MAG = 5;
boolean mag_on = false;
boolean paired = false;
boolean just_set = false;
bool mag_msg = false;

//HC-SR04 ----------------------------------------------------------------------------------------------
const int trigPin = 9;     
const int echoPin = 8;      
UltraSonicDistanceSensor distanceSensor(trigPin, echoPin);

//SD ---------------------------------------------------------------------------------------------------
const int pinCS = 10;

//BME280 -----------------------------------------------------------------------------------------------
Adafruit_BME280 bme;

//INA219 -----------------------------------------------------------------------------------------------
Adafruit_INA219 ina219;

//controls ---------------------------------------------------------------------------------------------
const int LED = A3;
const int time_interval = 5;          // interval in which the minutes will be delayed. Default is five minutes
unsigned long prevTimeElapsed = 0;

void setup() {
  Serial.begin(9600);
  pinMode(pinCS, OUTPUT);
  pinMode(LED, OUTPUT);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(PC, OUTPUT);
  pinMode(MAG, OUTPUT);
  pinMode(RTCinterrupt, INPUT_PULLUP);            // configure the interrupt pin using the built-in pullup resistor
  Serial.println("Executing EPM_data_collection.ino");

  // SD card initialization --------------------------------------------------------------------------------------------------------------------------
  if (!SD.begin())                               
  {
    digitalWrite(LED, HIGH);                      // LED remains on if SD card does not work
    Serial.println("no SD found");
    while (1);                                    
  }
  else
  {
    Serial.println("SD found");
  }

  // nRF24L01 initialization -------------------------------------------------------------------------------------------------------------------------
  radio.begin();                            
  radio.openWritingPipe(addresses[0]);            // set the address at which we will send the data (the rf controller's address)
  radio.openReadingPipe(1, addresses[1]);         // set the address at which we will receive the data. Change this if deploying multiple packages to
                                                  //   give each package a unique address
  radio.setPALevel(RF24_PA_MAX);                  // can set as minimum or maximum depending on the distance between the rf controller and package

  // RTC initializaiton ------------------------------------------------------------------------------------------------------------------------------
  RTC.begin();
  
  // initialize the alarms, clear the alarm flags, clear the alarm interrupts
  RTC.setAlarm(ALM1_MATCH_DATE, 0, 0, 0, 1);
  RTC.setAlarm(ALM2_MATCH_DATE, 0, 0, 0, 1);
  RTC.alarm(ALARM_1);
  RTC.alarm(ALARM_2);
  RTC.alarmInterrupt(ALARM_1, false);
  RTC.alarmInterrupt(ALARM_2, false);
  RTC.squareWave(SQWAVE_NONE);
  
  // get the current time from the RTC and set an alarm according to the time interval
  time_t t;                               
  t = RTC.get();                            
  
//  // uncomment to set the time interval units to seconds
//  if(second(t)< 60 - time_interval){
//    RTC.setAlarm(ALM1_MATCH_SECONDS , second(t)+time_interval, 0, 0, 0);
//  }
//  else {
//    RTC.setAlarm(ALM1_MATCH_SECONDS , second(t)-60+time_interval, 0, 0, 0);
//  }
  
  // uncomment to set the time interval units to minutes
  if (minute(t) < 60 - time_interval) {
    RTC.setAlarm(ALM1_MATCH_MINUTES , 0, minute(t) + time_interval, 0, 0);
  }
  else {
    RTC.setAlarm(ALM1_MATCH_MINUTES , 0, minute(t) - 60 + time_interval, 0, 0);
  }

  // clear the alarm flag and configure the interrupt operation
  RTC.alarm(ALARM_1);
  RTC.squareWave(SQWAVE_NONE);
  RTC.alarmInterrupt(ALARM_1, true);

  // BME initialization ------------------------------------------------------------------------------------------------------------------------------
  bme.begin(0x76);

  // Voltage regulator initialization -----------------------------------------------------------------------------------------------------------------
  uint32_t currentFrequency;
  ina219.begin();
}

void loop() {
  digitalWrite(LED, LOW);     // turn off LED before sleeping
  digitalWrite(PC, LOW);      // make sure magnet control circuit is off
  radio.startListening();     // set nRF24L01 as receiver
  delay(10);
  goSleep();
}

void goSleep() {
  Serial.println("Going to sleep...");
  delay(100);

  // initialize just_set as false. If we get a pairing command it will become true
  just_set = false;
  delay(100);

  // activate sleep mode, attach interrupt and assign a waking function to run
  sleep_enable();                               
  attachInterrupt(digitalPinToInterrupt(RTCinterrupt), RTCtrigger, LOW);
  attachInterrupt(digitalPinToInterrupt(nRFinterrupt), nRFtrigger, LOW);
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);              // set to full sleep mode   
  sleep_cpu();                                  

  // run the magnet toggle function if we received a command other than pairing
  if (mag_msg == true && just_set == false) {
    toggleMag();
  }

  // run the data collection function if RTC alarm goes off or if we have finished reading the nRF message
  if(just_set = false) {
    logData();                                   
  }
  
  // set the next alarm
  time_t t;
  t = RTC.get();
  
//  // uncomment to set the time interval units to seconds
//  if(second(t) < 60 - time_interval){
//    RTC.setAlarm(ALM1_MATCH_SECONDS , second(t)+time_interval, 0, 0, 0);
//  }
//  else {
//    RTC.setAlarm(ALM1_MATCH_SECONDS , second(t)-60+time_interval, 0, 0, 0);
//  }
  
  // uncomment to set the time interval units to minutes
  if (minute(t) < 60 - time_interval) {
    RTC.setAlarm(ALM1_MATCH_MINUTES , 0, minute(t) + time_interval, 0, 0);
  }
  else {
    RTC.setAlarm(ALM1_MATCH_MINUTES , 0, minute(t) - 60 + time_interval, 0, 0);
  }

  // clear the alarm flag
  RTC.alarm(ALARM_1);
}

void RTCtrigger() {
// this is the wake up function to run once the RTC interrupt is fired
  Serial.println("RTC interrupt fired");
  delay(100);
  sleep_disable();                        // disable sleep mode
  detachInterrupt(digitalPinToInterrupt(RTCinterrupt));          // clear the interrupt flag
}

void nRFtrigger() {                             
// this is the wake up function to run once the nRF interrupt is fired
  sleep_disable();
  detachInterrupt(digitalPinToInterrupt(nRFinterrupt));
  delay(2000);
  
  // determines if interrupt was caused by a transmission (tx), transmission failure (fail), or reception (rx)
  bool tx, fail, rx;
  radio.whatHappened(tx, fail, rx);                   

  // if we received a message, read the message and act accordingly
  if ( rx || radio.available()) {                     
    radio.read(&command, sizeof(command)); 
    delay(500);
    // if we got a pairing command:
    if (command == 1) {                                       
      radio.stopListening();
      delay(50);
      radio.write(&command, sizeof(command));     // send back a ping to the transmitter
      delay(50);
      radio.startListening();
      just_set = true;                            // change boolean so magnet is not toggled and data not collected
    }
    // if we got an ON command: 
    if (command == 2) {
      mag_on = true;
    }
    // if we got an OFF command: 
    else if (command == 3) {
      mag_on = false;
    }
    mag_msg = true;           // change boolean so toggleMag() is run (don't want to call within the wakeup function)
  }
}

void toggleMag() {
// this is the function that toggles the EPM

  // if we received an ON command
  if(mag_on == true) {
      digitalWrite(PC, HIGH);          // turn on the magnet control circuit
      delay(1500);
      for (int i = 0; i < 30; i++)     // pulse the PWM pin at 50 Hz
        {
          digitalWrite(MAG, HIGH);
          delay(2);
          digitalWrite(MAG, LOW);
          delay(18);
        }
        Serial.println("Mag on");
        delay(2000);
        digitalWrite(PC, LOW);         // turn off the magnet control circuit
    }

    // if we received an OFF command
    else if(mag_on == false) {
      digitalWrite(PC, HIGH);          // turn on the magnet control circuit
      delay(1500);
      for (int i = 0; i < 30; i++)     // pulse the PWM pin at 33 Hz
        {
          digitalWrite(MAG, HIGH);
          delay(1.5);
          digitalWrite(MAG, LOW);
          delay(19);
        }
        Serial.println("Mag off");
        delay(5000);
        digitalWrite(PC, LOW);         // turn off the magnet control circuit
    }
    mag_msg = false;
}

void logData() {
// this is the data collection function
  unsigned long timeElapsed = millis();
  Serial.println("Recording data...");
  digitalWrite(LED, HIGH);
  delay(10);

  // take five distance readings and average them
  float dist[5];
  float total = 0;
  float num = 0;
  for (int i = 0; i < 5; i++) {
    dist[i] = distanceSensor.measureDistanceCm();
    delay(500);
    if (dist[i] > 0) {
      total = total + dist[i];
      num++;
    }
  }
  float avgDist = total / num;

  // record the ambient temperature, humidity, pressure
  float temp = bme.readTemperature();
  float humidity = bme.readHumidity();
  float pressure = bme.readPressure() / 100.0F;
  delay(50);

  // record the voltage, current, and power draw from the LiPo
  float busvoltage = ina219.getBusVoltage_V();
  float current_mA = ina219.getCurrent_mA();
  float power_mW = ina219.getPower_mW();

  // print the data to the file that will be saved on the SD card
  File myFile = SD.open("001.csv", FILE_WRITE);    // change to the file name you want to store the data

  if (myFile)                             // tests if the file has opened
  {
    // write the RTC data
    time_t t = RTC.get();
    myFile.print(String(month(t)));
    myFile.print("/");
    myFile.print(String(day(t)));
    myFile.print("/");
    myFile.print(String(year(t)));
    myFile.print(" ");
    myFile.print(String(hour(t)));
    myFile.print(":");
    myFile.print(String(minute(t)));
    myFile.print(":");
    myFile.print(String(second(t)));
    myFile.print(" ");

    // write the USS data
    for (int i = 0; i < 5; i++) {
      myFile.print(dist[i]); myFile.print(" ");
    }
    myFile.print(avgDist); myFile.print(" ");

    // write the BME data
    myFile.print(temp); myFile.print(" ");
    myFile.print(humidity); myFile.print(" ");
    myFile.print(pressure); myFile.print(" ");

    // write the power data
    myFile.print(busvoltage); myFile.print(" ");
    myFile.print(current_mA); myFile.print(" ");
    myFile.print(power_mW); myFile.print(" ");

    myFile.println("");
    myFile.close();           // closes and saves the file to the SD card
    Serial.print("Complete! Elapsed time: "); Serial.print(timeElapsed - prevTimeElapsed); Serial.println(" ms");
    prevTimeElapsed = timeElapsed;
  }
  else
  {
    Serial.println("Error opening file");
    digitalWrite(LED, HIGH);                // LED will stay on if the file is not opening properly. 
    while (1);
  }
  delay(100);
  digitalWrite(LED, LOW);
}
