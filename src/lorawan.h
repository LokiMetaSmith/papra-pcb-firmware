#ifndef LORAWAN_H
#define LORAWAN_H

#include <Arduino.h>
#include "globals.h"

#if defined(STM32WL54CC)
#include <STM32LoRaWAN.h>
#else
#include <lmic.h>
#include <hal/hal.h>
#endif

void setupLoRaWAN();
void loopLoRaWAN();

#endif // LORAWAN_H
