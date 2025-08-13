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

// nRF ---------------------------------------------------------------------------------------------------
const int rfpinCS = 7;

// BME280 -----------------------------------------------------------------------------------------------
Adafruit_BME280 bme;

// INA219 -----------------------------------------------------------------------------------------------
Adafruit_INA219 ina219;

// controls ---------------------------------------------------------------------------------------------
const int LED = A3;
//constexpr time_t alarmInterval{5*60}; // wake up interval in seconds
unsigned long alarmInterval = 5 * 60; // 5 min default
unsigned long prevTimeElapsed = 0;

// Returns the sleep interval in seconds based on LiPo voltage
unsigned long getDynamicInterval(float voltage) {
  if (voltage >= 7.4) return 30*60;        // 30 min
  else if (voltage >= 7) return 60*60;  // 1 hr
  else return 12*60*60;                  // 12 hours
}

void setup() {
  Serial.begin(9600);
  pinMode(pinCS, OUTPUT);
  pinMode(LED, OUTPUT);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(RTCinterrupt, INPUT_PULLUP);  // configure the interrupt pin using the built-in pullup resistor

  // SD card initialization --------------------------------------------------------------------------------------------------------------------------
  if (!SD.begin())                               
  {
    digitalWrite(LED, HIGH);            // LED remains on if SD card does not work
    Serial.println("no SD found");
    while(true){
      error_blink();                                    
    }
  }
  else
  {
    Serial.println("SD found");
  }

  // RTC initializaiton ------------------------------------------------------------------------------------------------------------------------------
  // initialize the alarms to known values, clear the alarm flags, clear the alarm interrupt flags
  RTC.begin();
  RTC.setAlarm(DS3232RTC::ALM1_MATCH_DATE, 0, 0, 0, 1);
  RTC.setAlarm(DS3232RTC::ALM2_MATCH_DATE, 0, 0, 0, 1);
  RTC.alarm(DS3232RTC::ALARM_1);
  RTC.alarm(DS3232RTC::ALARM_2);
  RTC.alarmInterrupt(DS3232RTC::ALARM_1, false);
  RTC.alarmInterrupt(DS3232RTC::ALARM_2, false);
  RTC.squareWave(DS3232RTC::SQWAVE_NONE);
  
  // get the current time from the RTC and set an alarm according to the time interval
                            
  time_t t = RTC.get();                            
  time_t a = t + alarmInterval - t % alarmInterval;
  if (a <= t) a += alarmInterval;
  // set the alarm
  RTC.setAlarm(DS3232RTC::ALM1_MATCH_HOURS, second(a), minute(a), hour(a), 0);
  RTC.alarm(DS3232RTC::ALARM_1);    // clear the alarm flag
  RTC.alarmInterrupt(DS3232RTC::ALARM_1, true);

  // BME initialization ------------------------------------------------------------------------------------------------------------------------------
  bme.begin(0x76);

  // Voltage regulator initialization -----------------------------------------------------------------------------------------------------------------
  //uint32_t currentFrequency;
  ina219.begin();
}

void loop() {
  digitalWrite(LED, LOW);     // turn off LED before sleeping
  delay(10);
  goSleep();
}

void goSleep() {
  delay(100);

  // Put Arduino into sleep mode
  sleep_enable();                               
  attachInterrupt(digitalPinToInterrupt(RTCinterrupt), RTCtrigger, FALLING);
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);           
  sleep_cpu();                                  

  // Run the data logging function after waking
  logData();                                   

  // --- Read voltage and set dynamic alarm ---
  float busvoltage = ina219.getBusVoltage_V();
  alarmInterval = getDynamicInterval(busvoltage);

  // Set next alarm
  time_t t = RTC.get();
  time_t a = t + alarmInterval;
  RTC.setAlarm(DS3232RTC::ALM1_MATCH_HOURS, second(a), minute(a), hour(a), 0);
  RTC.alarm(DS3232RTC::ALARM_1);    // clear the alarm flag
}

void RTCtrigger() {
  // this is the wake up function to run once the RTC interrupt is fired
  //Serial.println("RTC interrupt fired");
  delay(100);

  sleep_disable();                        // disable sleep mode
  detachInterrupt(digitalPinToInterrupt(RTCinterrupt));          // clear the interrupt flag
}

void logData() {
  // this is the data collection function
  unsigned long timeElapsed = millis();
  //Serial.println("Recording data...");
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
    myFile.print(",");

    // write the USS data
    for (int i = 0; i < 5; i++) {
      myFile.print(dist[i]); myFile.print(",");
    }
    myFile.print(avgDist); myFile.print(",");

    // write the BME data
    myFile.print(temp); myFile.print(",");
    myFile.print(humidity); myFile.print(",");
    myFile.print(pressure); myFile.print(",");

    // write the power data
    myFile.print(busvoltage); myFile.print(",");
    myFile.print(current_mA); myFile.print(",");
    myFile.print(power_mW); myFile.print(",");

    myFile.println("");
    myFile.close();           // closes and saves the file to the SD card
    //Serial.print("Complete! Elapsed time: "); //Serial.print(timeElapsed - prevTimeElapsed); //Serial.println(" ms");
    prevTimeElapsed = timeElapsed;
  }
  else
  {
    //Serial.println("Error opening file");
    digitalWrite(LED, HIGH);                // LED will stay on if the file is not opening properly. 
    while(true){
      error_blink();
    }
  }
  delay(100);
  digitalWrite(LED, LOW);
}

void error_blink(){
  for(int i=0; i<5; i++){
    digitalWrite(LED, 1);
    delay(50);
    digitalWrite(LED, 0);
    delay(50);
  }
  delay(2000);
}
