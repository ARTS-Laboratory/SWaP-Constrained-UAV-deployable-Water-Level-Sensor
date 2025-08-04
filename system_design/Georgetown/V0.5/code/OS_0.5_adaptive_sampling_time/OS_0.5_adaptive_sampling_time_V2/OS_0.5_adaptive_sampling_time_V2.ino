#include <SD.h>              // for SD card module
#include <SPI.h>             // for SPI
#include <Wire.h>            // for I2C
#include <HCSR04.h>          // for the USS
#include <DS3232RTC.h>       // for the RTC https://github.com/JChristensen/DS3232RTC
DS3232RTC RTC;
#include <avr/sleep.h>       // for sleep mode
#include <Adafruit_Sensor.h>   // for BME 
#include <Adafruit_BME280.h>   // for BME
#include <Adafruit_INA219.h>   // for voltage monitor

// --- Pin Definitions ---
const int RTCinterrupt = 2;        // RTC interrupt from sleep mode on digital pin 2
const int trigPin = 9;   
const int echoPin = 8;   
const int pinCS = 10;              // SD card chip select
const int rfpinCS = 7;             // nRF chip select (if used)
const int LED = A3;

// --- Constants ---
#define SEALEVELPRESSURE_HPA (1013.25) // Constant for BME sensor

// --- Sensor Objects ---
UltraSonicDistanceSensor distanceSensor(trigPin, echoPin);
Adafruit_BME280 bme;
Adafruit_INA219 ina219;

// --- Global Variables ---
time_t alarmInterval; // Wake up interval in seconds. This is now a variable.
unsigned long prevTimeElapsed = 0;

void setup() {
  // Initialize Serial for debugging if needed
  // Serial.begin(9600);

  // --- Pin Modes ---
  pinMode(pinCS, OUTPUT);
  pinMode(LED, OUTPUT);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(RTCinterrupt, INPUT_PULLUP); // Configure the interrupt pin using the built-in pullup resistor

  // --- SD Card Initialization ---
  if (!SD.begin()) {
    digitalWrite(LED, HIGH); // LED remains on if SD card does not work
    // Serial.println("no SD found");
    while (1) {
      error_blink();
    }
  } else {
    // Serial.println("SD found");
  }

  // --- Sensor Initializations ---
  RTC.begin();
  bme.begin(0x76);
  ina219.begin();
  ina219.setCalibration_16V_400mA(); // For 0.1 ohm shunt resistor
  delay(10);

  // --- Set Initial Alarm Interval Based on Voltage ---
  float initial_busvoltage = ina219.getBusVoltage_V();
  if (initial_busvoltage > 7.0) {
    alarmInterval = 5; // 5 second interval if voltage is high
  } else {
    alarmInterval = 10; // 10 second interval if voltage is low
  }
  // Serial.print("Initial alarm interval set to: ");
  // Serial.println(alarmInterval);

  // --- RTC Initialization and First Alarm Setting ---
  // Initialize the alarms to known values, clear the alarm flags, clear the alarm interrupt flags
  RTC.setAlarm(DS3232RTC::ALM1_MATCH_DATE, 0, 0, 0, 1);
  RTC.setAlarm(DS3232RTC::ALM2_MATCH_DATE, 0, 0, 0, 1);
  RTC.alarm(DS3232RTC::ALARM_1);
  RTC.alarm(DS3232RTC::ALARM_2);
  RTC.alarmInterrupt(DS3232RTC::ALARM_1, false);
  RTC.alarmInterrupt(DS3232RTC::ALARM_2, false);
  RTC.squareWave(DS3232RTC::SQWAVE_NONE);
  
  // Get the current time from the RTC and set an alarm according to the time interval
  time_t t = RTC.get();
  time_t a = t + alarmInterval - t % alarmInterval;
  if (a <= t) a += alarmInterval;
  
  // Set the first alarm
  RTC.setAlarm(DS3232RTC::ALM1_MATCH_HOURS, second(a), minute(a), hour(a), 0);
  RTC.alarm(DS3232RTC::ALARM_1); // Clear the alarm flag
  RTC.alarmInterrupt(DS3232RTC::ALARM_1, true); // Enable alarm interrupt
}

void loop() {
  digitalWrite(LED, LOW); // Turn off LED before sleeping
  delay(100);
  goSleep();
}

/**
 * @brief Blinks the LED 5 times to indicate an error state.
 */
void error_blink() {
  for (int i = 0; i < 5; i++) {
    digitalWrite(LED, 1);
    delay(50);
    digitalWrite(LED, 0);
    delay(50);
  }
  delay(2000);
}

/**
 * @brief Handles the process of entering sleep, waking, logging data, and setting the next alarm.
 */
void goSleep() {
  // Serial.println("Going to sleep...");
  delay(100);

  // Activate sleep mode, attach interrupt and assign a waking function to run
  attachInterrupt(digitalPinToInterrupt(RTCinterrupt), RTCtrigger, FALLING);
  sleep_enable();
  set_sleep_mode(SLEEP_MODE_PWR_DOWN); // Set to full sleep mode
  sleep_cpu();

  // --- WAKE UP ---
  // CPU resumes execution here after interrupt
  
  // Run the data collection function
  logData();
  
  // Set the next alarm using the interval determined in logData()
  time_t t = RTC.get();
  time_t a = t + alarmInterval;
  RTC.setAlarm(DS3232RTC::ALM1_MATCH_HOURS, second(a), minute(a), hour(a), 0);
  RTC.alarm(DS3232RTC::ALARM_1); // Clear the alarm flag
}

/**
 * @brief Interrupt Service Routine (ISR) for the RTC alarm.
 * This function is called when the RTC interrupt is fired.
 */
void RTCtrigger() {
  // This is the wake up function to run once the RTC interrupt is fired
  // Keep this ISR as short as possible.
  // Serial.println("RTC interrupt fired");
  sleep_disable();
  detachInterrupt(digitalPinToInterrupt(RTCinterrupt));
}

/**
 * @brief Reads all sensors, determines the next alarm interval, and logs data to the SD card.
 */
void logData() {
  unsigned long timeElapsed = millis();
  // Serial.println("Recording data...");
  digitalWrite(LED, HIGH); // Turn on LED during data logging
  delay(10);

  // --- Read Sensors ---

  // Take five distance readings and average them
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
  float avgDist = (num > 0) ? (total / num) : 0;

  // Record the ambient temperature, humidity, pressure
  float temp = bme.readTemperature();
  float humidity = bme.readHumidity();
  float pressure = bme.readPressure() / 100.0F;
  delay(50);

  // Record the voltage, current, and power draw from the LiPo
  float busvoltage = ina219.getBusVoltage_V();
  float current_mA = ina219.getCurrent_mA();
  float power_mW = ina219.getPower_mW();

  // --- DYNAMICALLY SET NEXT ALARM INTERVAL ---
  if (busvoltage > 7.0) {
    alarmInterval = 5; // 5 second interval if voltage is high
  } else {
    alarmInterval = 10; // 10 second interval if voltage is low
  }
  // Serial.print("Next alarm interval will be: ");
  // Serial.println(alarmInterval);

  // --- Write Data to SD Card ---
  File myFile = SD.open("001.csv", FILE_WRITE);

  if (myFile) {
    // Write the RTC data
    time_t t = RTC.get();
    myFile.print(String(month(t))); myFile.print("/");
    myFile.print(String(day(t))); myFile.print("/");
    myFile.print(String(year(t))); myFile.print(" ");
    myFile.print(String(hour(t))); myFile.print(":");
    myFile.print(String(minute(t))); myFile.print(":");
    myFile.print(String(second(t))); myFile.print(",");

    // Write the USS data
    for (int i = 0; i < 5; i++) {
      myFile.print(dist[i]); myFile.print(",");
    }
    myFile.print(avgDist); myFile.print(",");

    // Write the BME data
    myFile.print(temp); myFile.print(",");
    myFile.print(humidity); myFile.print(",");
    myFile.print(pressure); myFile.print(",");

    // Write the power data
    myFile.print(busvoltage); myFile.print(",");
    myFile.print(current_mA); myFile.print(",");
    myFile.print(power_mW); myFile.print(",");

    myFile.println("");
    myFile.close(); // Closes and saves the file to the SD card
    // Serial.print("Complete! Elapsed time: "); Serial.print(timeElapsed - prevTimeElapsed); Serial.println(" ms");
    prevTimeElapsed = timeElapsed;
  } else {
    // Serial.println("Error opening file");
    digitalWrite(LED, HIGH); // LED will stay on if the file is not opening properly.
    while (1) {
      error_blink();
    }
  }
  delay(100);
  digitalWrite(LED, LOW);
}
