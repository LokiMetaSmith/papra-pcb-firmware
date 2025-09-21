// Implementation of calibration and command functions
#include "globals.h"
#include "calibration.h"
#include "tachometer.h" // For calculateRPM()
#include "main.h" // For getPressure()

void runCalibration() {
  Serial.println(F("Entering calibration mode..."));
  Serial.println(F("PWM,RPM,Pressure,Impedance"));

  // Turn off PID control during calibration
  // by setting fan speed directly.

  float prev_pressure = 0;
  int prev_pwm = 0;

  for (int pwm = minPWM10p; pwm <= 255; pwm += 10) {
    TCB0.CCMPH = pwm; // Direct register write for TCB0 PWM
    delay(2000); // Wait 2 seconds for system to stabilize

    uint32_t currentRPM = calculateRPM();
    float currentPressure = getPressure();

    // Calculate impedance
    float delta_pressure = currentPressure - prev_pressure;
    int delta_pwm = pwm - prev_pwm;
    float impedance = 0;
    if (delta_pressure > 0) {
      impedance = (float)delta_pwm / delta_pressure;
    }

    Serial.print(pwm);
    Serial.print(F(","));
    Serial.print(currentRPM);
    Serial.print(F(","));
    Serial.print(currentPressure);
    Serial.print(F(","));
    Serial.println(impedance);

    prev_pressure = currentPressure;
    prev_pwm = pwm;
  }

  // Calibration finished, return to normal operation
  TCB0.CCMPH = 0; // Turn off fan before resuming PID
  Serial.println(F("Calibration finished."));
}

void checkSerialCommands() {
  while (Serial.available() > 0) {
    char c = (char)Serial.read();
    if (c == '\n' || c == '\r') {
      serialCommand.trim();
      if (serialCommand.equalsIgnoreCase("calibrate")) {
        runCalibration();
      }
      serialCommand = "";
    } else {
      serialCommand += c;
    }
  }
}
