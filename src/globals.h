#ifndef GLOBALS_H
#define GLOBALS_H

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <EEPROM.h>

// BME280 I2C address is 0x76 or 0x77. Using 0x76 as an example.
#define BME280_I2C_ADDRESS 0x76

// OLED Display settings
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET    -1 // Reset pin # (or -1 if sharing Arduino reset pin)

// Struct for storing configuration in EEPROM
struct Config {
  uint32_t magic_number; // To check if EEPROM is initialized
  double Kp;
  double Ki;
  double Kd;
};

#include <Adafruit_SSD1306.h>

// Extern declarations for global variables defined in papracode.ino
extern Adafruit_BME280 bme;
extern Adafruit_SSD1306 display;
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
extern bool alerts_muted;
extern bool alert_active;

// Pin definitions for ATtiny3226 (20-pin)
const int led1        = PIN_PC0;
const int led2        = PIN_PC1;
const int led3        = PIN_PC2;
const int led4        = PIN_PC3;
const int PWMPin      = PIN_PB0; // TCA0 PWM output
const int buzzerPin   = PIN_PB1;
const int tachPin     = PIN_PA3;
const int analogPot   = PIN_PD0;
const int analogBatt  = PIN_PD1;
const int muteButtonPin = PIN_PD2;
// I2C pins are PA1 (SDA) and PA2 (SCL) - handled by Wire library
// UART pins are PB2 (TX) and PB3 (RX) - handled by Serial library

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
