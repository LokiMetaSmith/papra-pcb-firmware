// Implementation of calibration and command functions
#include "globals.h"
#include "serial_commands.h"
#include "tachometer.h" // For calculateRPM()
#include "main.h" // For getPressure()
#include "eeprom_config.h" // For saveConfig()

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
      if (serialCommand.length() == 0) return;

      // Handle 'calibrate' command
      if (serialCommand.equalsIgnoreCase("calibrate")) {
        runCalibration();
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
