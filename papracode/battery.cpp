#include "globals.h"
#include "battery.h"

void checkBattery() {
  // Average the battery reading over a few samples
  uint32_t current_reading = 0;
  const uint32_t numBatterySamples = 10;
  for (int i = 0; i < numBatterySamples; i++) {
    current_reading += analogRead(analogBatt);
  }
  uint32_t battery = current_reading / numBatterySamples;

  switch (battery) {
    case battADC78p ... battADCMax: // Full = 78% - 100%
      if (batteryState > batteryFull) {
        batteryState = batteryFull;
        digitalWrite(led2, LOW);
        digitalWrite(led3, LOW);
        digitalWrite(led4, LOW);
        minPWM = minPWMfull;
      }
      break;
    case battADC55p ... (battADC78p - 1): // 75% = 55% - 77%
      if (batteryState > battery75p) {
        batteryState = battery75p;
        digitalWrite(led2, LOW);
        digitalWrite(led3, LOW);
        digitalWrite(led4, HIGH);
        minPWM = minPWM75p;
      }
      break;
    case battADC33p ... (battADC55p - 1): // 50% = 33% - 54%
      if (batteryState > battery50p) {
        batteryState = battery50p;
        digitalWrite(led2, LOW);
        digitalWrite(led3, HIGH);
        digitalWrite(led4, HIGH);
        minPWM = minPWM50p;
      }
      break;
    case battADC10p ... (battADC33p - 1): // 25% = 10% - 32%
      if (batteryState > battery25p) {
        batteryState = battery25p;
        digitalWrite(led2, LOW); // Solid LED2
        digitalWrite(led3, HIGH);
        digitalWrite(led4, HIGH);
        minPWM = minPWM25p;
      }
      break;
    case battADC0p ... (battADC10p - 1): // 10% - Need to blink LED
      if (batteryState > battery10p) {
        batteryState = battery10p;
        digitalWrite(led3, HIGH);
        digitalWrite(led4, HIGH);
        minPWM = minPWM10p;
      }
      break;
    case battADCMin ... (battADC0p - 1): // Shutdown
      if (batteryState > batteryDead) {
        batteryState = batteryDead;
        digitalWrite(led2, HIGH);
        digitalWrite(led3, HIGH);
        digitalWrite(led4, HIGH);
        maxPWM = 0; // turn off fan - overrides the measuremnt from above
      }
      break;
  }

  // Blinking logic for low battery
  static int blinkCounter = 0;
  if (batteryState == battery10p) {
    if (blinkCounter++ > LEDFlashLoop) {
      digitalWrite(led2, !digitalRead(led2));
      blinkCounter = 0;
    }
  }
}
