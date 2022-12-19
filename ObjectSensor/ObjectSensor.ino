/**
  CIS3146 - Embedded Systems
  Coursework 1 - Arduino Alarm Clock

  @author Jamie Longbottom - 24452220
  @author Aiden Parker - 24631698
  @author Jack Smith - 24265241
*/

#include <LiquidCrystal.h>

// define pins
#define TRIG 11         // define trigger pin of sensor module to 11.
#define ECHO 12         // define echo pin of sensor module to 12.
#define TMP A0          // define temperature pin to A0.
#define PIEZO 8         // define speaker pin of sensor module to 8.
#define TIMEBUTTON 10   // define time button pin to 10
#define ALARMBUTTON 13  // define alarm button pin to 13
#define TIMESETDIAL A2  // define time set potentiometer

// further definitions needed
#define ALARM 9476
#define TIME 9478

// variables for timekeeping
int hour = 0, minute = 0, second = 0;
unsigned long timeLastUpdated = 0;

// variables for temp and distance sensors
double distance;
int tempC;

// variables for display update triggers
int previousTemp = 1000;            // impossible value for initial comparison
unsigned long tempUpdateTimer = 0;  // time since temp was last updated, reduces flickering
bool timeChanged = false;           // true when time display needs updating
bool sunMoonChanged = false;        // true when sun/moon needs updating

// variables for setting and silencing alarm
bool alarmSet = false;              // true when alarm is set
int alarmHour = 0, alarmMinute = 0; // time of current alarm
const int alarmStopDistance = 30;   // The maximum distance in cm to stop the alarm
int timePresses = 0;                // number of times time set button has been pressed
int alarmPresses = 0;               // number of times alarm set button has been pressed
unsigned long blinkTime = 500;      // time duration to blink time value being edited
int previousDisplayTime = 0;        // holds the previous display time to check for updates
unsigned long blinkTimer = 0;       // timer for edit blink
bool fullDisplay = false;           // toggle for if full display has just been shown or not

// variables for pulsing the alarm
const int NOTE_C4 = 262;       // frequency to emit from piezo.
unsigned long prevTime = 0;
const int duration = 500;      // duration to emit sound from piezo.
const int timeToDelay = 1000;  // time to delay the sound emitting.
bool stopFlag = true;
const int SPEED_OF_SOUND = 343;
/*
The climate of the enviroment is assumed to be dry air @ 20°C,
resulting in the speed of sound = 343 m/s.
*/

// variables to debounce time setting buttons
int timeButtonState = LOW;
int alarmButtonState = LOW;
unsigned long timeDebounce = 0;
unsigned long alarmDebounce = 0;
unsigned long debounceDelay = 50;

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
  lcd.begin(16, 2);             // initialise LCD screen
  pinMode(TRIG, OUTPUT);        // Set trigger pin to OUTPUT.
  pinMode(ECHO, INPUT);         // Set echo pin to INPUT.
  pinMode(TIMEBUTTON, INPUT);   // Set time pin button to INPUT
  pinMode(ALARMBUTTON, INPUT);  // Set alarm pin buttom to INPUT
  pinMode(TIMESETDIAL, INPUT);  // Set timeset dial pin to INPUT

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
  if((unsigned long)(millis() - timeLastUpdated) >= 1000) { // if a second has passed since last update
    second++; // increase second count
    timeLastUpdated = millis(); // time last updated -> now
    if(second == 60) { // when 60 secs reached
      second = 0; // reset second count
      minute++; // increase minute count
      if(minute == 60) { // when 60 mins reached
        minute = 0; // reset minute count
        hour++; // increase hour count
        if(hour == 8 || hour == 20) { // when sunrise/set
          sunMoonChanged = true; // sun/moon needs updating
        } else if(hour == 24) { // when 24hrs reached
          hour = 0; // reset hour count
        }
      }
      timeChanged = true; // minute updated so display needs updating
    }
  }

  /*--------------------
    Check button presses
  ----------------------*/

  if(buttonPressed(TIME, TIMEBUTTON, timeDebounce, timeButtonState)) { // time button press
    timePresses++;  // increase press count
    setTimeState();
  }

  if(buttonPressed(ALARM, ALARMBUTTON, alarmDebounce, alarmButtonState)) { // alarm button press
    alarmPresses++; // increase press count
    setAlarmState();
  }
  if(alarmPresses == 1 || alarmPresses == 2) {
    setAlarmState();
    // prevent further display updates when setting time
    return;
  }

  /*--------------
    Update display
  ----------------*/

  if(timeChanged) {
    timeChanged = false; // reset value
    updateTimeDisplay();
    if(sunMoonChanged) { // special case where sun/moon also needs updating
      sunMoonChanged = false; // reset value
      updateSunMoon();
    }
  }

  if(tempC != previousTemp && millis() - tempUpdateTimer >= 500) { // when temp has changed and enough time has passed since last update
    if(digitCount(previousTemp) > digitCount(tempC)) { // if display will need clearing to remove extra digit
      updateDisplay(); // whole display needs clearing and updating
    } else {
      updateTempDisplayValue(); // only the temp value needs updating
    }
    tempUpdateTimer = millis(); // last updated
    previousTemp = tempC; // previous displayed temp
  }

  /*------------
    Check alarm
  -------------*/

  if(shouldSoundAlarm()) {
    stopFlag = false; // allow alarm to sound
    soundAlarm();
  }
}

/*-----------------------------------
  Functions for updating the display
------------------------------------*/

/**
  Used to update entire display when a screen
  clear is required
*/
void updateDisplay() {
  // clear previous display
  lcd.clear();
  
  // display time
  updateTimeDisplay();
  
  //display day/night
  updateSunMoon();

  // display temp
  updateTempDisplayValue();
  printDegreesSymbol();

  // display alarm
  updateAlarmDisplay(false);
}

/**
  Prints the current time to the LCD display in a 24hr display
*/
void updateTimeDisplay() {
  // position cursor
  lcd.setCursor(0, 0);
  
  // print hour in 24hr display
  if(hour < 10) { // extra 0 needded at the start
    lcd.print(0);
  }
  lcd.print(hour);

  // print ':' seperator
  lcd.print(":");

  // print minute
  if(minute < 10) { // extra 0 needded at the start
    lcd.print(0);
  }
  lcd.print(minute);
}

/**
  Prints either a sun or moon image to the LCD display
  depending on the time, showing day or night clearly
*/
void updateSunMoon() {
  // position cursor
  lcd.setCursor(7,0);

  if(hour >=8 && hour < 20) { // daylight
    lcd.write(byte(0));
    lcd.write(byte(1));    
  } else { // night time
    lcd.write(byte(2));
    lcd.write(byte(3));
  }
}

/**
  Method to update the temperature shown on the LCD display
*/
void updateTempDisplayValue() {
  lcd.setCursor(14 - digitCount(tempC), 0);  // LCD alignment value
  lcd.print(tempC); // Print temp value to LCD
}

/**
  Prints the degrees unit to the LCD display in the
  appropriate position
*/
void printDegreesSymbol() {
  lcd.setCursor(14, 0);  // position cursor
  lcd.print((char)434);  // print degree symbol
  lcd.print("C");        // Print C char to LCD
}

/**
  Show the current alarm state, whether it is
  off, on or being set. When setting the alarm,
  a true value must be passed, otherwise it will
  be assumed the alarm is not set.
*/
void updateAlarmDisplay(bool settingAlarm) {
  if(settingAlarm) { // when setting alarm
    lcd.setCursor(0, 1); // position cursor
    lcd.print("Alarm - ");
    if(alarmHour < 10) { // extra 0 needded at the start
      lcd.print(0);
    }
    lcd.print(alarmHour);

    lcd.print(":");

    if(alarmMinute < 10) { // extra 0 needded at the start
      lcd.print(0);
    }
    lcd.print(alarmMinute);
  }

  else if(alarmSet) { // when alarm is set
    lcd.setCursor(0, 1); // position cursor
    lcd.print("Alarm on - ");

    // print alarm hour in 24hr display
    if(alarmHour < 10) { // extra 0 needded at the start
      lcd.print(0);
    }
    lcd.print(alarmHour);

    // print ':' seperator
    lcd.print(":");

    // print alarm minute
    if(alarmMinute < 10) { // extra 0 needded at the start
      lcd.print(0);
    }
    lcd.print(alarmMinute);
  }
  
  else { // when no alarm is set
    lcd.setCursor(3, 1); // position cursor
    lcd.print("Alarm off");
  }
}

/**
  This function returns the number of digits of
  an int value

  The formula to count the digits of an int
  in C++ was found at the following resource.

  GEEKSFORGEEKS, 2022. Program to count digits in an integer (4 Different Methods) [online].
  Available from: https://www.geeksforgeeks.org/program-count-digits-integer-3-different-methods/
  [Accessed 28 November 2022].
*/
int digitCount(int number) {
  return floor(log10(number) + 1);
}

/*----------------------
  Functions for sensors
-----------------------*/

// TODO reference
/**
  Computes the distance to an object, in feet, using the formula D = T * S / F.
  The climate of the enviroment is assumed to be dry air @ 20°C,
  resulting in the speed of sound = 343 m/s.

  Distance = (timeElapsed * (speedOfSound in cm) / convert to foot) / 2
*/
float calculateDistanceFoot(double timeElapsed) {
  return (timeElapsed * (SPEED_OF_SOUND * 0.0001) / 30.48) / 2;
}

// TODO reference
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
  5 40KHz waves are then sent from the module, with the echo pin receiving
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
  float vOut = vIn * (5000 / 1024.0); // Convert input to voltage output (vOut): reading * 5000mv / 1024 (input from A0 pin)
  int tempC = (vOut - 500) / 10;  // Convert to celcius using voltage - 500mv offset / 10
  return tempC;
}

/*-----------------------------
  Functions for sounding alarm 
------------------------------*/

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
    unsigned long delta = millis() - prevTime;  // difference between current time and previous recorded time.
    if (delta >= timeToDelay) {
      // if the current time - previous time is greater than
      prevTime = millis(); // update previous time to now
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

/*-------------------------------------
  Functions for time and alarm setting
--------------------------------------*/

/**
  Handles the setting of time on the LCD display.
  Called any time the time set button is pressed
  and appropriate action is taken.
*/
void setTimeState() {
  /*
    While loops are used to hold execution whilst time is being set.
    Each value of timePresses means a different function should be
    performed.
  
    1 press - set the hour
    2 presses - set the minutes
    3+ presses - return to normal clock, time is set
  */
  while(timePresses == 1) { // after 1 button press
    if(buttonPressed(TIME, TIMEBUTTON, timeDebounce, timeButtonState)) { // listen for additional button press
      timePresses++;
    }

    // set the hour based on the timeset dial
    hour = analogRead(TIMESETDIAL) * 23 / 1023;

    // blink the display on a timer to show which values are being altered
    if((millis() - blinkTimer >= 500)) { // blink every 0.5 seconds
      lcd.clear(); // clear the current display
      blinkTimer = millis(); // update timer to now - last blink
      if(previousDisplayTime != hour || !fullDisplay) { // show full display if the hour has changed or it was previously hidden
        updateTimeDisplay(); // show display
        fullDisplay = true; // update toggle to show full time is displayed
        previousDisplayTime = hour; // hold this hour as the previous display for blinking check
      }
      else { // when hour hasn't changed and the full time was just displayed
        // print time without the hour to create blink on these digits
        lcd.setCursor(2, 0);
        lcd.print(":"); 
        if(minute < 10) { // extra 0 needed before minutes
          lcd.print(0);
        }
        lcd.print(minute);
        fullDisplay = false; // update toggle to show hour was hidden
      }
    }
  }

  while(timePresses == 2) { // after 2 button presses
    if(buttonPressed(TIME, TIMEBUTTON, timeDebounce, timeButtonState)) { // listen for additional presses
      timePresses++;
    }

    // set the minute based on the timeset dial
    minute = long(analogRead(TIMESETDIAL)) * 59 / 1023;

    // blink the display on a timer to show which values are being altered
    if((millis() - blinkTimer >= 500)) { // blink every 0.5 seconds
      lcd.clear(); // clear the current display
      blinkTimer = millis(); // update timer to now - last blink
      if(previousDisplayTime != minute || !fullDisplay) { // show full display if the minute has changed or it was previously hidden
        updateTimeDisplay(); // show display
        fullDisplay = true; // update toggle to show full time is displayed
        previousDisplayTime = minute; // hold this minute as the previous display for blinking check
      }
      else { // when minute hasn't changed and the full time was just displayed
        // print time without the minute to create blink on these digits
        if(hour < 10) { // extra 0 needed before hour
          lcd.print(0);
        }
        lcd.print(hour);
        lcd.print(":"); 
        fullDisplay = false; // update toggle to show minute was hidden
      }
    }
  }

  // here time has been fully set
  second = 0; // reset seconds to 0 with new time
  timeLastUpdated = millis(); // time was updated here
  timePresses = 0; // reset presses so time can be set again
  updateDisplay(); // update entire display to show previously hidden features
}

/**
  Handles the setting of the alarm on the LCD display.
  Called any time the alarm set button is pressed or whilst
  setting is taking place (`alarmPress` values 1 and 2).

  This function needs continuously calling when setting the alarm
  so that while loops are not needed to hold the execution of the
  program, allowing the current time to still be updated in the
  `loop()` function
*/
void setAlarmState() {
  switch(alarmPresses) { // switch on the value of alarmPresses
    case 1: // 1st press - set hour
      alarmHour = analogRead(TIMESETDIAL) * 23 / 1023; // set the alarm hour based on the timeset dial
      if((millis() - blinkTimer >= 500)) { // blink every 0.5 secs
        lcd.clear(); // clear current display
        blinkTimer = millis(); // update timer to now - last blink
        if(previousDisplayTime != alarmHour || !fullDisplay) { // show full display if the alarm hour has changed or it was previously hidden
          updateAlarmDisplay(true); // show display
          fullDisplay = true; // update toggle to show full time is displayed
          previousDisplayTime = alarmHour; // hold this hour as the previous display for blinking check
        }
        else { // when hour hasn't changed and the full time was just displayed
          // print time without the hour to create blink on these digits
          lcd.setCursor(0, 1);
          lcd.print("Alarm -   :"); 
          if(alarmMinute < 10) { // extra 0 needed at start of minutes
            lcd.print(0);
          }
          lcd.print(alarmMinute);
          fullDisplay = false; // update toggle to show hour was hidden
        }
      }
      break; // do not continue execution
    
    case 2: // 2nd press - set minute
      alarmMinute = long(analogRead(TIMESETDIAL)) * 59 / 1023; // set the alarm minute based on the timeset dial
      if((millis() - blinkTimer >= 500)) { // blink every 0.5 secs
        lcd.clear(); // clear current display
        blinkTimer = millis(); // update timer to now - last blink
        if(previousDisplayTime != alarmMinute || !fullDisplay) { // show full display if the alarm minute has changed or it was previously hidden
          updateAlarmDisplay(true); // show display
          fullDisplay = true; // update toggle to show full time is displayed
          previousDisplayTime = alarmMinute; // hold this minute as the previous display for blinking check
        }
        else { // when minute hasn't changed and the full time was just displayed
          // print time without the minute to create blink on these digits
          lcd.clear();
          lcd.setCursor(0, 1);
          lcd.print("Alarm - "); 
          if(alarmHour < 10) { // extra 0 needed before hour
            lcd.print(0);
          }
          lcd.print(alarmHour);
          lcd.print(":");
          fullDisplay = false; // update toggle to show hour was hidden
        }
      }
      break; // do not continue execution

    case 3: // 3rd press - set alarm to go off
      alarmSet = true;
      updateDisplay(); // exiting alarm setting so show full display
      break;  // do not continue execution

    default: // any further presses or errors
      alarmSet = false; // turn off alarm
      alarmPresses = 0; // reset button presses
      updateDisplay(); // alarm display needs to be turned off requiring a screen clear and other elements redrawing
  }
}

/**
  This function checks whether a button has been pressed and
  will also remove noise from double readings.

  This code was researched at the following resource:

  JAMES, MICHAEL, 2022. DEBOUNCING A BUTTON WITH ARDUINO [online].
  Available from: https://www.programmingelectronics.com/debouncing-a-button-with-arduino/
  [Accessed on 19 December 2022].
*/
bool buttonPressed(int timeOrAlarm, int buttonPin, int debounceTimer, int buttonState) {
  bool pressed = false; // returned true if valid button press
  int reading = digitalRead(buttonPin); // get button reading
  if((millis() - debounceTimer) > debounceDelay) { // if enough time has passed since last read (avoid double readings)
    if(reading != buttonState) { // if button has changed state
      buttonState = reading; // update state
      if(buttonState == HIGH) { // if pressed
        pressed = true;
      }
      // update global variables of corresponding buttons
      if(timeOrAlarm == TIME) {
        timeButtonState = reading;
        timeDebounce = millis();
      } else if(timeOrAlarm == ALARM) {
        alarmButtonState = reading;
        alarmDebounce = millis();
      }
    }
  }
  return pressed;
}
