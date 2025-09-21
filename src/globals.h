#ifndef GLOBALS_H
#define GLOBALS_H

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

// BME280 I2C address is 0x76 or 0x77. Using 0x76 as an example.
#define BME280_I2C_ADDRESS 0x76

// Extern declarations for global variables defined in papracode.ino
extern Adafruit_BME280 bme;
extern int batteryState;
extern int loopDelay;
extern uint32_t fanPWM;
extern uint32_t minPWM;
extern uint32_t maxPWM;
extern uint32_t rawADC;
extern volatile unsigned long lastTachPulseTime;
extern volatile unsigned long tachPulseInterval;
extern uint32_t fanRPM;
extern double pid_setpoint;
extern double Kp, Ki, Kd;
extern double pid_integral, pid_derivative, pid_last_error;
extern unsigned long pid_last_time;
enum BreathState { STATE_INHALE, STATE_EXHALE };
extern BreathState currentBreathState;
extern double ipap_pressure;
extern double epap_pressure;
extern double initial_pressure;
extern double pressure_history[10];
extern int pressure_history_index;
extern unsigned long last_inhale_time;
extern float respiratory_rate;
extern String serialCommand;
extern bool apnea_alert_active;

// Pin definitions (can be defined in header as they are const)
const int analogBatt  = PIN_A1;
const int analogPot   = PIN_A2;
const int tachPin     = PIN_PA3;
const int led4        = PIN_PA4;
const int led2        = PIN_PA6;
const int led3        = PIN_PA7;
const int PWMPin      = PIN_PA5;

// Constants (can be defined in header)
const int battADCMax  =  1023;
const int battADCFull =   899;
const int battADC78p  =   864;
const int battADC55p  =   816;
const int battADC33p  =   781;
const int battADC10p  =   712;
const int battADC0p   =   618;
const int battADCMin  =     0;
const int minPWMfull =  50;
const int minPWM75p  =  55;
const int minPWM50p  =  61;
const int minPWM25p  =  70;
const int minPWM10p  = 126;
const int batteryCheck = 7;
const int batteryFull  = 6;
const int battery75p   = 5;
const int battery50p   = 4;
const int battery25p   = 3;
const int battery10p   = 2;
const int batteryDead  = 1;
const int LEDFlashLoop  = 25;

#endif // GLOBALS_H
