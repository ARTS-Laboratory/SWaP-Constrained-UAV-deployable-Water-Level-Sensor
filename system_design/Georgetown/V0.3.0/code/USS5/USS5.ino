 /*
 * Written by Corinne Smith
 * July 9 2021
 * Modified from ArduinoGetStarted, www.ArduinoGetStarted.com
 * Modified from Dejan Nedelkowski, www.HowToMechatronics.com
 * Modified from ArduinoLearning, www.ArduinoLearning.com
 */

#include <SD.h>                         // for SD card module
#include <SPI.h>                        // for SD card and nRF
#include <Wire.h>                       // for RTC
#include <HCSR04.h>                     // for the USS
#include <RF24.h>                       // for the nRF
#include <DS3232RTC.h>                  // RTC Library https://github.com/JChristensen/DS3232RTC
#include <avr/sleep.h>                  // for sleep mode
#include <Adafruit_Sensor.h>            // for BME 
#include <Adafruit_BME280.h>            // for BME
#include <Adafruit_INA219.h>            // for voltage monitor


#define CE 7
#define CSN 6
#define MAX_DAT_SIZE 16
#define interruptPin 2                  // RTC interrupt from sleep mode
#define SEALEVELPRESSURE_HPA (1013.25)


//RF ----------------------------------------------------------------------------------------------------
  RF24 radio(CE, CSN); //CE, CSN
  const byte rAddress[6] = "00003";
  const byte tAddress[6] = "00001";
  int runOnce = 0;
  boolean switch_state = 0;
  String converter;

  //initialize file for NRF 
    File myFile;
    size_t readField(File* myFile, char* str, size_t size, char* delim) {
    char ch;
    size_t n = 0;
      while ((n + 1) < size && myFile->read(&ch, 1) == 1) {
        // Delete CR.
        if (ch == '\r') {
          continue;
        }
        str[n++] = ch;
        if (strchr(delim, ch)) {
          break;
        }
      }
      str[n] = '\0';
      return n;
    }

//Ultrasonic ----------------------------------------------------------------------------------------------
  const int trigPin = 9;         
  const int echoPin = 8;         
  UltraSonicDistanceSensor distanceSensor(trigPin, echoPin);  

//SD ------------------------------------------------------------------------------------------------------
  const int pinCS = 10;          //defines ChipSelect pin for SD module

//BME -----------------------------------------------------------------------------------------------------
  Adafruit_BME280 bme;

//Voltage Regulator ---------------------------------------------------------------------------------------
  Adafruit_INA219 ina219;

//control pins
  const int LED = 4;
  const int magPin = 3;
  const int magPower = 5;
  const int time_interval = 5;  // interval in which the minutes will be delayed
  const int buzz = 7;



void setup() {
    Serial.begin(9600);

    pinMode(buzz, OUTPUT);
    pinMode(pinCS, OUTPUT);            
    pinMode(LED, OUTPUT);
    pinMode(trigPin, OUTPUT);
    pinMode(echoPin, INPUT);
    pinMode(interruptPin,INPUT_PULLUP); //Set pin d2 to input using the buildin pullup resistor
    pinMode(magPower, OUTPUT);
    pinMode(magPin, OUTPUT);

// nRF24L01 initialization --------------------------------------------------------------------------------------------------------------------------
    radio.begin();
    radio.openWritingPipe(tAddress);
    radio.openReadingPipe(1, rAddress);   //Setting the address at which we will receive the data
    radio.setPALevel(RF24_PA_MAX);       //You can set this as minimum or maximum depending on the distance between the transmitter and receiver.
    if (radio.setDataRate(RF24_1MBPS) == true)
      {
         Serial.println("DATA RATE SET");
      }
//    radio.startListening();     //Makes module a receiver

// SD card initialization --------------------------------------------------------------------------------------------------------------------------
   if (!SD.begin())               // tests if SD module began
      {
        digitalWrite(LED, HIGH);
        Serial.println("no SD found");
       while(1);                  // LED remains on if SD card does not work
     }
     else
     {
      Serial.println("SD found");
     }

// RTC initializaiton ------------------------------------------------------------------------------------------------------------------------------
    RTC.begin();
    // initialize the alarms to known values, clear the alarm flags, clear the alarm interrupt flags
    RTC.setAlarm(ALM1_MATCH_DATE, 0, 0, 0, 1);
    RTC.setAlarm(ALM2_MATCH_DATE, 0, 0, 0, 1);
    RTC.alarm(ALARM_1);
    RTC.alarm(ALARM_2);
    RTC.alarmInterrupt(ALARM_1, false);
    RTC.alarmInterrupt(ALARM_2, false);
    RTC.squareWave(SQWAVE_NONE);
    time_t t;                               // create a temporary time variable so we can set the time and read the time from the RTC
    t = RTC.get();                            // Gets the current time of the RTC
    if(second(t)<55){
      RTC.setAlarm(ALM1_MATCH_SECONDS , second(t)+time_interval, 0, 0, 0);
    }
    else {
      RTC.setAlarm(ALM1_MATCH_SECONDS , second(t)-60+time_interval, 0, 0, 0);
    }
    // clear the alarm flag
    RTC.alarm(ALARM_1);
    // configure the INT/SQW pin for "interrupt" operation (disable square wave output)
    RTC.squareWave(SQWAVE_NONE);
    // enable interrupt output for Alarm 1
    RTC.alarmInterrupt(ALARM_1, true);
    
// BME initialization ------------------------------------------------------------------------------------------------------------------------------
    bme.begin(0x76);

// Voltage regulator initialization -----------------------------------------------------------------------------------------------------------------
    uint32_t currentFrequency;
    ina219.begin();
  }

void loop() {
    digitalWrite(LED, LOW);
    delay(10);
    goSleep();
  }

void goSleep(){
    sleep_enable();                               // Enable sleep mode
    attachInterrupt(0, wakeUp, LOW);              // attach an interrupt to pin 2 and run the wakeUp() function upon waking
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);          // Set full sleep mode
    sleep_cpu();                                  // Activate sleep mode
    logData();                                    // Run the logData() function after the wakeUp() function
    time_t t;
    t=RTC.get(); 
    //Set New Alarm
    if(minute(t)<55){
        RTC.setAlarm(ALM1_MATCH_MINUTES , 0, minute(t)+time_interval, 0, 0);
      }
    else {
        RTC.setAlarm(ALM1_MATCH_MINUTES , 0, minute(t)-60+time_interval, 0, 0);
    }
    // clear the alarm flag
    RTC.alarm(ALARM_1);
   }
  
void wakeUp() {
    Serial.println("RTC interrupt fired");
    delay(1000);
    sleep_disable();                        // Disable sleep mode
    detachInterrupt(0);                     // Remove the interrupt from pin 2
}

void logData() {
      digitalWrite(LED, HIGH);    
      delay(10);
      
    // for the USS Module
      float dist[5];
      float total = 0;
      float num = 0;
      for(int i=0; i<5; i++) {
        dist[i] = distanceSensor.measureDistanceCm();
        delay(500);
        if(dist[i] > 0) {
          total = total + dist[i];
          num++;
        }
      }
      float avgDist = total/num;

    // for the BME
      float temp = bme.readTemperature();
      float humidity = bme.readHumidity();
      float pressure = bme.readPressure() / 100.0F;
      delay(50);

    // for the voltage regulator
      float busvoltage = ina219.getBusVoltage_V();
      float current_mA = ina219.getCurrent_mA();
      float power_mW = ina219.getPower_mW();

      while(busvoltage < 6.65) {
        Serial.print("Voltage: "); Serial.println(busvoltage);
        tone(buzz, 500, 3000);
        delay(1000);
        busvoltage = ina219.getBusVoltage_V();
      }

    // Now print the data to the file that will be saved on the SD card
      myFile = SD.open("001.csv", FILE_WRITE);

      if (myFile)                         // tests if the file has opened
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
          for(int i=0; i<5; i++) {
            myFile.print(dist[i]); myFile.print(" ");
          }
          myFile.print(avgDist); myFile.print(" ");

          // write the BME data
          myFile.print(temp); myFile.print(" ");
          myFile.print(humidity); myFile.print(" ");
          myFile.print(pressure); myFile.print(" ");
 

          // write the voltage data
          myFile.print(busvoltage); myFile.print(" ");
          myFile.print(current_mA); myFile.print(" ");
          myFile.print(power_mW); myFile.print(" ");

          myFile.println("");
          myFile.close();                   // closes and saves the file to the SD card
        }
      else
        {
          digitalWrite(LED, HIGH);         // if the file doesn't open indicate with the LED 
          while(1);
        }
  delay(100);
  digitalWrite(LED, LOW);
  
}


void getnRF() {
  digitalWrite(LED, HIGH);
  if (radio.available())     //Looking for incoming data and returns true or false accordingly
  {
    digitalWrite(LED, HIGH);      //Indicates that a connection has been established
    radio.read(&switch_state, sizeof(switch_state));      //Reads the boolean signal
    if (switch_state == HIGH)     //This means that the master has asked for the package's data
    {
      readAndSend();
    }
  }
  else {
    digitalWrite(LED, LOW);
  }
}


void readAndSend() {
  unsigned long currentTime;
  unsigned long initialTime;
  unsigned long totalTime;
  initialTime = millis();
  radio.stopListening();
  // Initialize the SD.
  SD.begin();
  // Create or open the file.
  char csvFileName[] = "001.csv";
  myFile = SD.open(csvFileName, FILE_READ);
  //if (!myFile) errorHalt("OPEN FAILED");
  size_t n;      // Length of returned field with delimiter.
  char str[50];  // Must hold longest field with delimiter and zero byte.
  int j = 0;
  int g = 0;
  int r = 0;
  while (true) { //Reads and stores each line
    n = readField(&myFile, str, sizeof(str), ",\n");
    // done if Error or at EOF.
    if (n == 0) break;
    // Print the type of delimiter.
    if (str[n - 1] == ',' || str[n - 1] == '\n') {
      if (r != 0) {
        // Remove the delimiter.
        str[n - 1] = 0;
        String converter = str;
      }
    }
    else
    {
      // At eof, too long, or read error.  Too long is error.
      //Serial.println(file.available() ? F("Error") : F("eof:   "));
    }
  }
  myFile.close();
  int c = 0;
  while (c < 1) {
    if (radio.write(&converter, sizeof(converter)) == false) {    //Checks to make sure the chunk was sent successfully
      if (c == 0) {                                                   //If a send fails, it will retry until it is successful
        c = 0;
      }
    }
    else {
      c++;
    }
    //Serial.print("COMPLETION: "); Serial.print(((float)c / (float)6945) * 100); Serial.print("%"); Serial.println();
  }
  currentTime = millis();
  totalTime = (currentTime - initialTime) / 1000;
}
