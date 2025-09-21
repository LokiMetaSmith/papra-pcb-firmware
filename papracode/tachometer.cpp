// Implementation of tachometer functions
#include "globals.h"
#include "tachometer.h"

uint32_t calculateRPM() {
  unsigned long interval;
  unsigned long last_pulse_time;
  // Safely read the volatile variables
  noInterrupts();
  interval = tachPulseInterval;
  last_pulse_time = lastTachPulseTime;
  interrupts();

  // Check if the fan is stalled (no pulse for a long time)
  // Check for interval > 0 to avoid division by zero
  if ( (micros() - last_pulse_time > 1000000) || (interval == 0) ) {
    return 0;
  } else {
    // RPM = (1 / (interval_in_us * 1e-6)) * (1 rev / 2 pulses) * (60 s / 1 min)
    // RPM = 30,000,000 / interval
    return 30000000UL / interval;
  }
}

void tach_isr() {
  unsigned long now = micros();
  tachPulseInterval = now - lastTachPulseTime;
  lastTachPulseTime = now;
}
