// Implementation of PID controller functions
#include "globals.h"
#include "pid_controller.h"

int computePID(double current_pressure) {
  unsigned long now = millis();
  double time_change = (double)(now - pid_last_time);

  // This PID implementation is time-dependent.
  // We only re-calculate if a certain amount of time has passed.
  if (time_change < 20) { // Sample time of 20ms
    return fanPWM; // Return last computed value
  }

  double error = pid_setpoint - current_pressure;

  // Integral term with anti-windup
  pid_integral += error * time_change;
  if (pid_integral > maxPWM) pid_integral = maxPWM;
  if (pid_integral < minPWM) pid_integral = minPWM;

  pid_derivative = (error - pid_last_error) / time_change;

  double output = Kp * error + Ki * pid_integral + Kd * pid_derivative;

  pid_last_error = error;
  pid_last_time = now;

  // Clamp the output to the valid PWM range
  if (output > maxPWM) output = maxPWM;
  if (output < minPWM) output = minPWM;

  return (int)output;
}
