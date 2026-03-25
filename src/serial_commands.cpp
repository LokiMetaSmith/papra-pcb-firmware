// Implementation of calibration and command functions
#include "globals.h"
#include "serial_commands.h"
#include "tachometer.h" // For calculateRPM()
#include "main.h" // For getPressure()
#include "eeprom_config.h" // For saveConfig()
#include <PID_AutoTune.h>

// --- Autotune Variables ---
byte ATuneMode = 2; // 2 = Ziegler-Nichols PI, 3 = Ziegler-Nichols PID
unsigned int ATuneSampleTime = 50; // How often to run the autotuner
double ATuneStartValue = 128; // Initial PWM output
double ATuneStep = 50; // PWM step size for the tuning cycle
double ATuneNoise = 1.0; // Noise band
unsigned int ATuneLookback = 60; // Lookback time in seconds

PID_AutoTune tuner = PID_AutoTune();

void runAutotune() {
  Serial.println(F("Starting PID Autotune..."));
  Serial.println(F("This will take a few minutes. The fan will oscillate."));

  // Set up the tuner
  tuner.SetControlType(ATuneMode);
  tuner.SetNoiseBand(ATuneNoise);
  tuner.SetOutputStep(ATuneStep);
  tuner.SetLookbackSec((int)ATuneLookback);

  unsigned long last_autotune_run = millis();
  double autotune_output = ATuneStartValue;

  // The autotune loop
  while (tuner.running()) {
    if (millis() - last_autotune_run >= ATuneSampleTime) {
      last_autotune_run = millis();

      double input = getPressure();
      int val = tuner.Runtime(input);

      if (val != 0) {
        // Tuning is finished
        break;
      }

      autotune_output = tuner.GetOutput();
      analogWrite(PWMPin, autotune_output);
    }
  }

  // Get the results
  Kp = tuner.GetKp();
  Ki = tuner.GetKi();
  Kd = tuner.GetKd();

  Serial.println(F("PID Autotune finished."));
  Serial.print(F("New Kp: ")); Serial.println(Kp);
  Serial.print(F("New Ki: ")); Serial.println(Ki);
  Serial.print(F("New Kd: ")); Serial.println(Kd);
  Serial.println(F("Saving new values to EEPROM..."));
  saveConfig();

  // Turn fan off
  analogWrite(PWMPin, 0);
}

void runCalibration() {
  Serial.println(F("Entering calibration mode..."));
  Serial.println(F("PWM,RPM,Pressure,Impedance"));

  // Turn off PID control during calibration
  // by setting fan speed directly.

  float prev_pressure = 0;
  int prev_pwm = 0;

  for (int pwm = minPWM10p; pwm <= 255; pwm += 10) {
    analogWrite(PWMPin, pwm);
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
  analogWrite(PWMPin, 0); // Turn off fan before resuming PID
  Serial.println(F("Calibration finished."));
}

void runNewFilterCalibration() {
  Serial.println(F("Entering new filter calibration mode..."));

  // Turn off PID control during calibration
  // by setting fan speed directly.

  float prev_pressure = 0;
  int prev_pwm = 0;

  double total_impedance = 0;
  int num_samples = 0;

  for (int pwm = minPWM10p; pwm <= 255; pwm += 10) {
    analogWrite(PWMPin, pwm);
    delay(2000); // Wait 2 seconds for system to stabilize

    float currentPressure = getPressure();

    // Calculate impedance
    float delta_pressure = currentPressure - prev_pressure;
    int delta_pwm = pwm - prev_pwm;
    float impedance = 0;
    if (delta_pressure > 0) {
      impedance = (float)delta_pwm / delta_pressure;
      total_impedance += impedance;
      num_samples++;
    }

    prev_pressure = currentPressure;
    prev_pwm = pwm;
  }

  if (num_samples > 0) {
    baseline_impedance = total_impedance / num_samples;
    Serial.print(F("Calibration finished. New baseline impedance: "));
    Serial.println(baseline_impedance);
    saveConfig();
  } else {
    Serial.println(F("Calibration failed to calculate impedance."));
  }

  // Calibration finished, return to normal operation
  analogWrite(PWMPin, 0); // Turn off fan before resuming PID
}

void checkSerialCommands() {
  while (Serial.available() > 0) {
    char c = (char)Serial.read();
    if (c == '\n' || c == '\r') {
      serialCommand.trim();
      if (serialCommand.length() == 0) return;

      // Handle 'calibrate' command
      if (serialCommand.equalsIgnoreCase("calibrate")) {
        runCalibration();
      }
      // Handle 'autotune' command
      else if (serialCommand.equalsIgnoreCase("autotune")) {
        runAutotune();
      }
      // Handle 'newfilter' command
      else if (serialCommand.equalsIgnoreCase("newfilter")) {
        runNewFilterCalibration();
      }
      // Handle 'save' command
      else if (serialCommand.equalsIgnoreCase("save")) {
        saveConfig();
      }
      // Handle 'set' commands (e.g., "set kp 2.5")
      else if (serialCommand.toLowerCase().startsWith("set ")) {
        // Find the space after "set"
        int firstSpace = serialCommand.indexOf(' ');
        // Find the space between the variable and the value
        int secondSpace = serialCommand.indexOf(' ', firstSpace + 1);

        if (secondSpace > firstSpace) {
          String varName = serialCommand.substring(firstSpace + 1, secondSpace);
          String varValueStr = serialCommand.substring(secondSpace + 1);
          double varValue = varValueStr.toFloat();

          if (varName.equalsIgnoreCase("kp")) {
            Kp = varValue;
            Serial.print(F("Set Kp = ")); Serial.println(Kp);
          } else if (varName.equalsIgnoreCase("ki")) {
            Ki = varValue;
            Serial.print(F("Set Ki = ")); Serial.println(Ki);
          } else if (varName.equalsIgnoreCase("kd")) {
            Kd = varValue;
            Serial.print(F("Set Kd = ")); Serial.println(Kd);
          } else {
            Serial.println(F("Unknown variable. Use kp, ki, or kd."));
          }
        } else {
          Serial.println(F("Invalid set command. Use: set <var> <value>"));
        }
      }
      else {
        Serial.print(F("Unknown command: "));
        Serial.println(serialCommand);
      }

      serialCommand = ""; // Clear for next command
    } else {
      // Don't buffer ridiculously long commands
      if (serialCommand.length() < 50) {
        serialCommand += c;
      }
    }
  }
}
