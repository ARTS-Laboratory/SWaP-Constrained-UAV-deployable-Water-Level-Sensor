#include <SD.h>                         // for SD card module
#include <DS3232RTC.h>                  // for the RTC https://github.com/JChristensen/DS3232RTC
DS3232RTC RTC;
#include <avr/sleep.h>                  // for sleep mode
#include <Adafruit_Sensor.h>            // for BME 
#include <Adafruit_BME280.h>            // for BME
#include <Adafruit_INA219.h>            // for voltage monitor

#define RTCinterrupt 2            // RTC interrupt from sleep mode on digital pin 2
//#define SEALEVELPRESSURE_HPA (1013.25)  // constant for bme

#define DEBUG_SERIAL 0

// HC-SR04 ----------------------------------------------------------------------------------------------
#define trigPin 9
#define echoPin 8

// SD ---------------------------------------------------------------------------------------------------
#define pinCS 10

// nRF ---------------------------------------------------------------------------------------------------
//#define rfpinCS 7

// BME280 -----------------------------------------------------------------------------------------------
Adafruit_BME280 bme;

// INA219 -----------------------------------------------------------------------------------------------
Adafruit_INA219 ina219;

// controls ---------------------------------------------------------------------------------------------
#define LED A3

int alarmInterval = 5 * 60; // 5 min default

// Returns the sleep interval in seconds based on LiPo voltage
unsigned long getDynamicInterval(float voltage) {
  if (voltage >= 8) return 15 * 60;      // 30 min
  else if (voltage >= 7.4) return 15 * 60; // 60 min
  else return 15 * 60;                // 2 hours
}

void setup() {
#if DEBUG_SERIAL
  Serial.begin(9600);
#endif
  pinMode(pinCS, OUTPUT);
  pinMode(LED, OUTPUT);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(RTCinterrupt, INPUT_PULLUP);  // configure the interrupt pin using the built-in pullup resistor

  // SD card initialization --------------------------------------------------------------------------------------------------------------------------
  if (!SD.begin())
  {
    digitalWrite(LED, HIGH);            // LED remains on if SD card does not work
#if DEBUG_SERIAL
    Serial.println("no SD found");
#endif
    while (true) {
      error_blink();
    }
  }
  else
  {
#if DEBUG_SERIAL
    Serial.println("SD found");
#endif
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

  // Voltage monitor initialization -----------------------------------------------------------------------------------------------------------------
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

long measureDistanceCm() {
  // Trigger pulse
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  // Read echo (timeout protects against hang)
  long duration = pulseIn(echoPin, HIGH, 30000UL); // 30 ms timeout

  if (duration == 0) {
    return -1;  // no echo
  }

  // Sound speed: ~0.0343 cm/us; /2 because out and back
  long distance = duration * 0.0343 / 2.0;
  return distance;
}

void logData() {
  // this is the data collection function

  digitalWrite(LED, HIGH);
  delay(10);

  // take five distance readings and average them
  float dist[5];
  float total = 0;
  float num = 0;
  for (int i = 0; i < 5; i++) {
    long d = measureDistanceCm();
    dist[i] = d;
    delay(500);
    if (d > 0) {
      total += d;
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

    // Date
    myFile.print(month(t));
    myFile.print('/');
    myFile.print(day(t));
    myFile.print('/');
    myFile.print(year(t));
    myFile.print(' ');

    // Time
    myFile.print(hour(t));
    myFile.print(':');
    myFile.print(minute(t));
    myFile.print(':');
    myFile.print(second(t));
    myFile.print(',');


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

    myFile.println();
    myFile.close();           // closes and saves the file to the SD card

  }
  else
  {
    digitalWrite(LED, HIGH);                // LED will stay on if the file is not opening properly.
    while (true) {
      error_blink();
    }
  }
  delay(100);
  digitalWrite(LED, LOW);
}

void error_blink() {
  for (int i = 0; i < 5; i++) {
    digitalWrite(LED, 1);
    delay(50);
    digitalWrite(LED, 0);
    delay(50);
  }
  delay(2000);
}
