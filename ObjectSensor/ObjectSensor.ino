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
#define SPEED_OF_SOUND 343  // The climate of the enviroment is assumed to be dry air @ 20°C, resulting in the speed of sound = 343 m/s.

// variables for timekeeping
int hour = 0, minute = 0;
unsigned long timeLastUpdated = 0;

int distanceLimit = 30;  // The maximum distance in cm for an object in the US sensors vicinity.

unsigned long prevTime;
const int duration = 500;      // duration to emit sound from piezo.
const int timeToDelay = 1000;  // time to delay the sound emitting.
bool stopFlag = false;

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
  /*-------------
    Update time
  -------------*/
  if((unsigned long)(millis() - timeLastUpdated) >= 60000) {
    minute++;
    timeLastUpdated = millis();
    if(minute == 60) {
      minute = 0;
      hour++;
      if(hour == 24) {
        hour = 0;
      }
    }
  }

  lcd.clear();
  updateTimeDisplay();
  delay(1000);

  // double distance = getDistanceInCM();  // calculate distance.

  // while (!stopFlag) {          // <-- this is needed
  //   distance = getDistanceInCM();  // update the distance once note has played.
  //   int tempC = calculateRoomTemp();
  //   displayDistAndTemp(distance, tempC);
  //   // while object not been detected.
  //   if (distance < distanceLimit) {
  //     // If the object is within the limit e.g. 30cm, stop alarm.
  //     stopAlarm();  // Stop the alarm.
  //   } else {
  //     // else an object has not been detected.
  //     long currentTime = millis();          // current execution time of program.
  //     long delta = currentTime - prevTime;  // difference between current time and previous recorded time.
  //     if (currentTime - prevTime >= timeToDelay) {
  //       // if the current time - previous time is greater than
  //       prevTime += timeToDelay;         // add delay time to previous time.
  //       tone(PIEZO, NOTE_C4, duration);  // emit sound.
  //     }
  //   }
  // }

  // int tempC = calculateRoomTemp(); //calculate temperature.
  // displayDistAndTemp(distance, tempC); // display time and temperature to LCD.
}

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
  lcd.print(" ");
  lcd.write(byte(2));
  lcd.write(byte(3));
}

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

/**
  Method to stop emitting alarm noise.
*/
void stopAlarm() {
  stopFlag = true;  // object detected
  noTone(PIEZO);    // stop playing sound on speaker
}

/**
  Method to print the time and temperature to the LCD.
*/
void displayDistAndTemp(int distance, int tempC) {
  lcd.clear();           // Clear screen to reset value
  lcd.print(distance);   // print the distance
  lcd.setCursor(11, 0);  // LCD alignment value
  lcd.print(tempC);      // Print temp value to LCD
  lcd.print((char)434);  // Enter char value (degree symbol)
  lcd.print("C");        // Print C char to LCD
  lcd.setCursor(16, 0);  // LCD alignment value
}