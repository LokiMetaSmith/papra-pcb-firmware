#include "globals.h"
#include "serial_commands.h"
#include "tachometer.h" // For calculateRPM()
#include "main.h" // For getPressure()
#include "eeprom_config.h" // For saveConfig()
#include <PID_AutoTune.h>

float runCalibration(); // Forward declaration

// --- Autotune Variables ---
byte ATuneMode = 2;
unsigned int ATuneSampleTime = 50;
double ATuneStartValue = 128;
double ATuneStep = 50;
double ATuneNoise = 1.0;
unsigned int ATuneLookback = 60;
PID_AutoTune tuner = PID_AutoTune();

void runAutotune() {
  Serial.println(F("Starting PID Autotune..."));
  tuner.SetControlType(ATuneMode);
  tuner.SetNoiseBand(ATuneNoise);
  tuner.SetOutputStep(ATuneStep);
  tuner.SetLookbackSec((int)ATuneLookback);
  unsigned long last_autotune_run = millis();
  while (tuner.running()) {
    if (millis() - last_autotune_run >= ATuneSampleTime) {
      last_autotune_run = millis();
      double input = getPressure();
      if (tuner.Runtime(input) != 0) {
        break;
      }
      analogWrite(PWMPin, tuner.GetOutput());
    }
  }
  Kp = tuner.GetKp();
  Ki = tuner.GetKi();
  Kd = tuner.GetKd();
  Serial.println(F("PID Autotune finished."));
  saveConfig();
  analogWrite(PWMPin, 0);
}

float runCalibration() {
  Serial.println(F("Entering calibration mode..."));
  Serial.println(F("PWM,RPM,Pressure,Impedance"));

  float prev_pressure = 0;
  int prev_pwm = 0;
  double impedance_accumulator = 0;
  int impedance_samples = 0;

  for (int pwm = minPWM10p; pwm <= 255; pwm += 10) {
    analogWrite(PWMPin, pwm);
    delay(2000);

    uint32_t currentRPM = calculateRPM();
    float currentPressure = getPressure();
    float impedance = 0;

    if (prev_pwm > 0) { // Don't calculate for the first step
        float delta_pressure = currentPressure - prev_pressure;
        int delta_pwm = pwm - prev_pwm;
        if (delta_pressure > 0) {
            impedance = (float)delta_pwm / delta_pressure;
            impedance_accumulator += impedance;
            impedance_samples++;
        }
    }

    Serial.print(pwm); Serial.print(F(","));
    Serial.print(currentRPM); Serial.print(F(","));
    Serial.print(currentPressure); Serial.print(F(","));
    Serial.println(impedance);

    prev_pressure = currentPressure;
    prev_pwm = pwm;
  }

  analogWrite(PWMPin, 0);
  Serial.println(F("Calibration finished."));

  if (impedance_samples > 0) {
    return impedance_accumulator / impedance_samples;
  } else {
    return 0; // Return 0 if no valid impedance could be measured
  }
}

void checkSerialCommands() {
  while (Serial.available() > 0) {
    char c = (char)Serial.read();
    if (c == '\n' || c == '\r') {
      serialCommand.trim();
      if (serialCommand.length() == 0) return;

      if (serialCommand.equalsIgnoreCase("calibrate")) {
        runCalibration();
      }
      else if (serialCommand.equalsIgnoreCase("autotune")) {
        runAutotune();
      }
      else if (serialCommand.equalsIgnoreCase("newfilter")) {
        Serial.println(F("New filter installed. Running baseline calibration..."));
        baseline_filter_impedance = runCalibration();
        Serial.print(F("New baseline impedance is: ")); Serial.println(baseline_filter_impedance);
        saveConfig();
      }
      else if (serialCommand.equalsIgnoreCase("save")) {
        saveConfig();
      }
      else if (serialCommand.toLowerCase().startsWith("set ")) {
        int firstSpace = serialCommand.indexOf(' ');
        int secondSpace = serialCommand.indexOf(' ', firstSpace + 1);

        if (secondSpace > firstSpace) {
          String varName = serialCommand.substring(firstSpace + 1, secondSpace);
          String varValueStr = serialCommand.substring(secondSpace + 1);
          double varValue = varValueStr.toFloat();

          if (varName.equalsIgnoreCase("kp")) Kp = varValue;
          else if (varName.equalsIgnoreCase("ki")) Ki = varValue;
          else if (varName.equalsIgnoreCase("kd")) Kd = varValue;

          Serial.print(F("Set ")); Serial.print(varName); Serial.print(" = "); Serial.println(varValue);
        } else {
          Serial.println(F("Invalid set command. Use: set <var> <value>"));
        }
      }
      else {
        Serial.print(F("Unknown command: "));
        Serial.println(serialCommand);
      }

      serialCommand = "";
    } else {
      if (serialCommand.length() < 50) {
        serialCommand += c;
      }
    }
  }
}
