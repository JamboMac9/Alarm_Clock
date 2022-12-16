/**
  CIS3146 - Embedded Systems
  Coursework 1 - Arduino Alarm Clock

  @author Jamie Longbottom - 24452220
  @author Aiden Parker - 24631698
  @author Jack Smith - 24265241
*/

#include <LiquidCrystal.h>

// define pins
#define TRIG 11             // define trigger pin of sensor module to 11.
#define ECHO 12             // define echo pin of sensor module to 12.
#define TMP A0              // define temperature pin to A0.
#define PIEZO 8             // define speaker pin of sensor module to 8.

// further definitions needed
#define NOTE_C4 262         // frequency to emit from piezo.
#define SPEED_OF_SOUND 343 
/*
  The climate of the enviroment is assumed to be dry air @ 20°C,
  resulting in the speed of sound = 343 m/s.
*/

// variables for timekeeping
int hour = 0, minute = 0, second = 0;
unsigned long timeLastUpdated = 0;

// variables for temp and distance sensors
double distance;
int tempC;

// variables for setting and silencing alarm
bool alarmSet = false; // true when alarm is set
int alarmHour = 0, alarmMinute = 0; // time of current alarm
const int alarmStopDistance = 30;  // The maximum distance in cm to stop the alarm

// variables for pulsing the alarm
unsigned long prevTime;
const int duration = 500;      // duration to emit sound from piezo.
const int timeToDelay = 1000;  // time to delay the sound emitting.
bool stopFlag = true;

LiquidCrystal lcd(2, 3, 4, 5, 6, 7); // initialise the lcd screen

// custom symbols for lcd display
byte sunL[8] = {
  0b00010,
  0b00000,
  0b01001,
  0b00011,
  0b00011,
  0b01001,
  0b00000,
  0b00010
};

byte sunR[8] = {
  0b01000,
  0b00000,
  0b10010,
  0b11000,
  0b11000,
  0b10010,
  0b00000,
  0b01000,
};

byte moonL[8] = {
  0b00000,
  0b00011,
  0b00000,
  0b00000,
  0b00000,
  0b00100,
  0b00011,
  0b00000
};

byte moonR[8] = {
  0b00000,
  0b11000,
  0b11100,
  0b01100,
  0b01100,
  0b11100,
  0b11000,
  0b00000
};

void setup() {
  lcd.begin(16, 2);
  pinMode(TRIG, OUTPUT);  // Set trigger pin to OUTPUT.
  pinMode(ECHO, INPUT);   // Set echo pin to INPUT.

  // create custom chars
  lcd.createChar(0, sunL);
  lcd.createChar(1, sunR);
  lcd.createChar(2, moonL);
  lcd.createChar(3, moonR);
}

void loop() {
  /*--------------------
    Update sensor inputs
  ----------------------*/

  distance = getDistanceInCM();  // calculate distance.
  tempC = calculateRoomTemp(); //calculate temperature.

  /*------------
    Update time
  -------------*/
  if((unsigned long)(millis() - timeLastUpdated) >= 1000) {
    second++;
    timeLastUpdated = millis();
    if(second == 60) {
      second = 0;
      minute++;
      if(minute == 60) {
        minute = 0;
        hour++;
        if(hour == 24) {
          hour = 0;
        }
      }
    }
  }

  /*------------
    Check alarm
  -------------*/

  if(shouldSoundAlarm()) {
    stopFlag = false;
    soundAlarm();
  }

  /*-------------------
    Update LCD Display
  --------------------*/

  updateDisplay();
  delay(1000);
}

/*-----------------------------------
  Functions for updating the display
------------------------------------*/

/**
  Used to update entire display when a screen
  clear is required
*/
void updateDisplay() {
  lcd.clear(); // clear previous display
  
  // display time
  updateTimeDisplay();
  
  //display day/night
  lcd.setCursor(7,0);
  printSunOrMoon();

  // display temp
  updateTempDisplayValue();
  printDegreesSymbol();

  // display alarm
  updateAlarmDisplay();
}

/**
  Prints the current time to the LCD display in a 24hr display
*/
void updateTimeDisplay() {
  // print hour in 24hr display
  if(hour < 10) {
    lcd.print(0);
  }
  lcd.print(hour);

  // print ':' seperator
  lcd.print(":");

  // print minute
  if(minute < 10) {
    lcd.print(0);
  }
  lcd.print(minute);
}

/**
  Prints either a sun or moon image to the LCD display
  depending on the time, showing day or night clearly
*/
void printSunOrMoon() {
  if(hour >=8 && hour <= 20) {
    lcd.write(byte(0));
    lcd.write(byte(1));    
  } else {
    lcd.write(byte(2));
    lcd.write(byte(3));
  }
}

/**
  Method to update the temperature shown on the LCD display
*/
void updateTempDisplayValue() {
  // get number of digits
  int digitCount = floor(log10(tempC) + 1);
  /*
  * The above formula to count the digits of an int
  * in C++ was taken from the following resource. This
  * formula was required to set the position of text so
  * that it could be right-aligned to stop the 'cm' ending
  * from moving, making the display easier to read.
  *
  * GEEKSFORGEEKS, 2022. Program to count digits in an integer (4 Different Methods) [online].
  * Available from: https://www.geeksforgeeks.org/program-count-digits-integer-3-different-methods/
  * [Accessed 28 November 2022].
  */
  lcd.setCursor(14 - digitCount, 0);  // LCD alignment value
  lcd.print(tempC);      // Print temp value to LCD
}

/**
  Prints the degrees unit to the LCD display in the
  appropriate position
*/
void printDegreesSymbol() {
  lcd.setCursor(14, 0);
  lcd.print((char)434);  // Enter char value (degree symbol)
  lcd.print("C");        // Print C char to LCD
}

/**
  Show if the alarm is set or not. If the alarm
  is set, show the alarm time
*/
void updateAlarmDisplay() {

  if(alarmSet) { // when alarm is primed
    lcd.setCursor(0, 1); // position cursor
    lcd.print("Alarm on - ");

    // print alarm hour in 24hr display
    if(alarmHour < 10) {
      lcd.print(0);
    }
    lcd.print(alarmHour);

    // print ':' seperator
    lcd.print(":");

    // print alarm minute
    if(alarmMinute < 10) {
      lcd.print(0);
    }
    lcd.print(alarmMinute);
  } else { // when no alarm is set
    lcd.setCursor(3, 1);
    lcd.print("Alarm off");
  }
}

/*----------------------
  Functions for sensors
-----------------------*/

/**
  Computes the distance to an object, in feet, using the formula D = T * S / F.
  The climate of the enviroment is assumed to be dry air @ 20°C,
  resulting in the speed of sound = 343 m/s.

  Distance = (timeElapsed * (speedOfSound in cm) / convert to foot) / 2
*/
float calculateDistanceFoot(double timeElapsed) {
  return (timeElapsed * (SPEED_OF_SOUND * 0.0001) / 30.48) / 2;
}

/**
  Computes the distance to an object, in centimetres, using the formula D = T * S.
  The climate of the enviroment is assumed to be dry air @ 20°C,
  resulting in the speed of sound = 343 m/s.

  Distance = (timeElapsed * (speedOfSound in cm)) / 2
*/
float calculateDistanceCM(double timeElapsed) {
  return (timeElapsed * (SPEED_OF_SOUND * 0.0001)) / 2;  // 343 * 0.0001 = 0.0343 = 34,300
}

/**
  Ultrasonic module used is HC-SR04. Emits a 40KHz ultrasonic pulse.
  When the Trigger pin is at a HIGH state, a 10 µs pulse is emitted.
  8 40KHz waves are then sent from the module, with the echo pin receiving
  the bounce back pulses, timing the duration of each recieved pulse in microseconds.
*/
void sendTrigPulse() {
  digitalWrite(TRIG, LOW);   //cease emitting.
  delayMicroseconds(2);
  digitalWrite(TRIG, HIGH);  // Emit pulse.
  delayMicroseconds(5);
  digitalWrite(TRIG, LOW);   //cease emitting.
}

/**
  Sets the echo pin on the ultrasonic sensor to high and collects 
  the time it took to acknolwedge the return pulse wave and returns it.
*/
double getEchoTime() {
  return pulseIn(ECHO, HIGH);
}

/**
  This method initatites the trigger pulses and calls the getEchoTime() method to retrieve the duration
  of the reflected wave, this distance of the object is then calculated in centimeters and returned.
*/
double getDistanceInCM() {
  sendTrigPulse();                       // emit ultrasonic waves.
  double echoTime = getEchoTime();       // time taken for wave to be detected.
  return calculateDistanceCM(echoTime);  // return the distance in centimeters.
}

/**
  Calculates the current room temperature in °C using the TMP sensor
*/
int calculateRoomTemp() {
  /* Start of temp value loop - using analog to digital converter (ADC) in 5v pin */
  int vIn = analogRead(TMP);  // Reading voltage input (vIn) from TMP sensor
  // Convert input to voltage output (vOut): reading * 5000mv / 1024 (input from A0 pin)
  float vOut = vIn * (5000 / 1024.0);
  int tempC = (vOut - 500) / 10;  // Convert to celcius using voltage - 500mv offset / 10
  return tempC;
}

/*------------------------------------
  Functions for time and alarm setting 
--------------------------------------*/

/**
  Determines if the alarm should sound or not
*/
bool shouldSoundAlarm() {
  if(alarmSet && (hour == alarmHour) && (minute == alarmMinute) && (second == 0)) {
    // alarm is set and at the alarm time
    return true;
  } else if(!stopFlag) {
    // alarm has gone off and not yet been cancelled
    return true;
  } else {
    // no alarm sound needed
    return false;
  }
}

/**
  Sounds the alarm and checks if alarm needs to be silenced
*/
void soundAlarm() {
  if (distance < alarmStopDistance) {
    // If the object is within the limit e.g. 30cm, stop alarm.
    stopAlarm();  // Stop the alarm.
  } else {
    // else an object has not been detected.
    long currentTime = millis();          // current execution time of program.
    long delta = currentTime - prevTime;  // difference between current time and previous recorded time.
    if (currentTime - prevTime >= timeToDelay) {
      // if the current time - previous time is greater than
      prevTime += timeToDelay;         // add delay time to previous time.
      tone(PIEZO, NOTE_C4, duration);  // emit sound.
    }
  }
}

/**
  Method to stop emitting alarm noise.
*/
void stopAlarm() {
  noTone(PIEZO);    // stop playing sound on speaker
  stopFlag = true;  // object detected
}