
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
constexpr time_t alarmInterval{60*5}; // wake up interval in seconds
unsigned long prevTimeElapsed = 0;

void setup() {
  Serial.begin(9600);
  pinMode(pinCS, OUTPUT);
  pinMode(11, OUTPUT);
  pinMode(12, OUTPUT);
  pinMode(13, OUTPUT);
  
  pinMode(LED, OUTPUT);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(RTCinterrupt, INPUT_PULLUP);  // configure the interrupt pin using the built-in pullup resistor
  
  digitalWrite(11,0);
  digitalWrite(12,0);
  digitalWrite(13,0);
  digitalWrite(pinCS,0);
  // SD card initialization --------------------------------------------------------------------------------------------------------------------------
  

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
  ina219.setCalibration_16V_400mA(); //for 0.1 ohm shunt res used
}

void loop() {
  digitalWrite(LED, LOW);     // turn off LED before sleeping
  delay(1000);
  float busvoltage = ina219.getBusVoltage_V();
  float current_mA = ina219.getCurrent_mA();
  float power_mW = ina219.getPower_mW();

  Serial.print(busvoltage); Serial.print(",");
  Serial.print(current_mA); Serial.print(",");
  Serial.println(power_mW); 
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
void goSleep() {
  Serial.println("Going to sleep...");
  delay(100);

  // activate sleep mode, attach interrupt and assign a waking function to run                              
  attachInterrupt(digitalPinToInterrupt(RTCinterrupt), RTCtrigger, FALLING);
  sleep_enable(); 
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);              // set to full sleep mode   
  sleep_cpu();                                  

  // run the data collection function after the waking function
                                
  
  // set the next alarm
  time_t t= RTC.get();
  time_t a = t + alarmInterval;
  // set the alarm
  RTC.setAlarm(DS3232RTC::ALM1_MATCH_HOURS, second(a), minute(a), hour(a), 0);
  RTC.alarm(DS3232RTC::ALARM_1);    // clear the alarm flag
}

void RTCtrigger() {
  // this is the wake up function to run once the RTC interrupt is fired
  Serial.println("RTC interrupt fired");
  delay(100);

  sleep_disable();                        // disable sleep mode
  detachInterrupt(digitalPinToInterrupt(RTCinterrupt));          // clear the interrupt flag
}
