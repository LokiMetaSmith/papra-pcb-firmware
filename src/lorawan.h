#ifndef LORAWAN_H
#define LORAWAN_H

#include <Arduino.h>
#include <lmic.h>
#include <hal/hal.h>
#include "globals.h"

void setupLoRaWAN();
void loopLoRaWAN();

#endif // LORAWAN_H
