// Header for tachometer functions
#ifndef TACHOMETER_H
#define TACHOMETER_H

#include <Arduino.h>

// Function prototypes
uint32_t calculateRPM();
void tach_isr();

#endif // TACHOMETER_H
