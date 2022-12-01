#include <LiquidCrystal.h>
#include "pitches.h"

LiquidCrystal lcd(12, 11, 2, 3, 4, 5);

// Set notes from pitches.h for alarm sound
int alarm[] = {
  NOTE_C4, NOTE_G3, NOTE_C4, NOTE_G3, NOTE_C4, NOTE_G3, NOTE_C4, NOTE_G3
};

void setup() {
lcd.begin(16, 2);
}

void loop() {
  /* Start of temp value loop - using analog to digital converter (ADC) in 5v pin */
  int vIn = analogRead(A0);   // Reading voltage input (vIn) from TMP sensor
  // Convert input to voltage output (vOut): reading * 5000mv / 1024 (input from A0 pin)
  float vOut = vIn * (5000 / 1024.0);
  int tempC = (vOut - 500) / 10; // Convert to celcius using voltage - 500mv offset / 10

  lcd.clear();            // Clear screen to reset value
  lcd.setCursor(11, 0);   // LCD alignment value
  lcd.print(tempC);       // Print temp value to LCD
  lcd.print((char)434);   // Enter char value (degree symbol)
  lcd.print("C");         // Print C text to LCD
  lcd.setCursor(16, 0);   // LCD alignment value
  delay(5000);            // Delay reset for 5000ms
  /* End of temp value loop */

  /* Start of alarm tone loop */
  for (int note = 0; note < 8; note++) {
    tone(13, alarm[note], 1000);  // Set tone (pin 13, declare alarm array, duration of note)
    delay(500);                   // Delay 500ms between next loop of tone
  }
  /* End of alarm tone loop */
}