/*
  papracode
  Main file for the PAPRA firmware.
  This file contains the main setup() and loop() functions,
  and the definitions for global variables.
*/
#include "globals.h"
#include "tachometer.h"
#include "pid_controller.h"
#include "breath_model.h"
#include "alerts.h"
#include "serial_commands.h"
#include "battery.h"
#include "main.h"
#include "eeprom_config.h"
#include "input.h"

// --- Global Variable Definitions ---
// These are declared as 'extern' in globals.h
Adafruit_BME280 bme;
int batteryState = batteryCheck;
uint32_t fanPWM = 0;
uint32_t minPWM = minPWM10p;
uint32_t maxPWM = 255;
uint32_t rawADC = 0;
volatile unsigned long lastTachPulseTime = 0;
volatile unsigned long tachPulseInterval = 0;
uint32_t fanRPM = 0;
double pid_setpoint = 50.0;
double Kp = 2.0, Ki = 0.5, Kd = 1.0;
double pid_integral = 0, pid_derivative = 0, pid_last_error = 0;
unsigned long pid_last_time = 0;
BreathState currentBreathState = STATE_INHALE;
double ipap_pressure = 60.0;
double epap_pressure = 30.0;
double initial_pressure = 0.0;
double pressure_history[10];
int pressure_history_index = 0;
unsigned long last_inhale_time = 0;
float respiratory_rate = 0.0;
String serialCommand;
bool apnea_alert_active = false;
bool alerts_muted = false;
bool alert_active = false;
// --- End of Global Variable Definitions ---


// the setup routine runs once when you press reset:
void setup() {
  // Initialize digital pins
  pinMode(tachPin, INPUT_PULLUP);
  pinMode(led1, OUTPUT);
  pinMode(led2, OUTPUT);
  pinMode(led3, OUTPUT);
  pinMode(led4, OUTPUT);
  pinMode(buzzerPin, OUTPUT);
  pinMode(PWMPin, OUTPUT);
  pinMode(muteButtonPin, INPUT_PULLUP);

  // Configure TCA0 for standard PWM on PWMPin (PB0)
  TCA0.SINGLE.CTRLB = TCA_SINGLE_WGMODE_SINGLESLOPE_gc; // Single slope PWM
  TCA0.SINGLE.PER = 0xFF; // 8-bit resolution
  TCA0.SINGLE.CTRLA = TCA_SINGLE_CLKSEL_DIV4_gc | TCA_SINGLE_ENABLE_bm; // Enable with prescaler

  // Initialize I2C on default pins (PA1, PA2)
  Wire.begin();

  // Attach tachometer interrupt
  attachInterrupt(digitalPinToInterrupt(tachPin), tach_isr, FALLING);

  // Initialize timers
  pid_last_time = millis();
  last_inhale_time = millis();

  // Set initial pin states
  digitalWrite(led1, HIGH);
  digitalWrite(led2, HIGH);
  digitalWrite(led3, HIGH);
  digitalWrite(led4, HIGH);
  digitalWrite(buzzerPin, LOW);
  analogWrite(PWMPin, maxPWM); // Turn on fan 100%

  Serial.begin(115200);
  Serial.println(F("Starting up PAPRA..."));

  loadConfig(); // Load settings from EEPROM or save defaults

  // Initialize BME280 sensor
  if (!bme.begin(BME280_I2C_ADDRESS)) {
    Serial.println(F("Could not find a valid BME280 sensor, check wiring!"));
    while (1) delay(10); // Halt
  }

  // Get initial pressure reading to use as a baseline for gauge pressure
  initial_pressure = bme.readPressure();

  // Initialize pressure history buffer
  for(int i=0; i<10; i++) {
    pressure_history[i] = 0;
  }
}

// the loop routine runs over and over again forever:
void loop() {
  delay(25); // Main loop delay

  checkSerialCommands();
  checkMuteButton();
  checkAlerts();

  // If an alert is active, override normal LED display with a visual alert
  if (alert_active) {
    triggerVisualAlert();
    triggerAudibleAlert();
  } else {
    // Otherwise, run the normal battery status display
    checkBattery();
  }

  // --- Tachometer RPM Calculation ---
  fanRPM = calculateRPM();

  // --- Main Control Logic ---
  float pressure = getPressure(); // Read pressure once per loop

  if (apnea_alert_active) {
    // --- EMERGENCY VENTILATION MODE ---
    // WARNING: This is a simplistic, non-medical, experimental feature.
    unsigned long cycle_time = 4500; // 4.5 second total cycle (~13.3 BPM)
    unsigned long inhale_time = 1500; // 1.5 second inhale
    unsigned long time_in_cycle = millis() % cycle_time;

    if (time_in_cycle < inhale_time) {
      pid_setpoint = ipap_pressure; // Use last known IPAP
    } else {
      pid_setpoint = 0; // Drop to zero pressure
    }
    fanPWM = computePID(pressure);

  } else {
    // --- BI-LEVEL PAP MODE ---
    updateBreathModel(pressure);

    rawADC = analogRead(analogPot);
    ipap_pressure = 40.0 + ((double)rawADC / 1023.0) * (50.0);
    epap_pressure = 40.0; // Fixed EPAP for now

    if (currentBreathState == STATE_INHALE) {
      pid_setpoint = ipap_pressure;
    } else {
      pid_setpoint = epap_pressure;
    }
    fanPWM = computePID(pressure);
  }

  // Set the fan speed from either mode
  analogWrite(PWMPin, fanPWM);

  // --- Serial Debug Output ---
  Serial.print(" pressure = ");      Serial.print((int)pressure);
  Serial.print(" setpoint = ");      Serial.print((int)pid_setpoint);
  Serial.print(" state = ");         Serial.print(currentBreathState == STATE_INHALE ? "Inhale" : "Exhale");
  Serial.print(" RR = ");            Serial.print(respiratory_rate);
  Serial.print(" pwm = ");           Serial.print(fanPWM);
  Serial.print(" rpm = ");           Serial.print(fanRPM);
  Serial.print(" battery_state = "); Serial.print(batteryState);
  Serial.println();
}

/**
 * @brief Reads the gauge pressure from the BME280 I2C sensor.
 *
 * @return float The gauge pressure in Pascals (Pa), relative to the
 *               pressure at startup.
 */
float getPressure() {
  // Return the current pressure minus the initial pressure to get gauge pressure.
  return bme.readPressure() - initial_pressure;
}
