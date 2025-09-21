// Implementation of alert system functions
#include "globals.h"
#include "alerts.h"

// Placeholder for the alert system
void checkAlerts() {
  // Apnea Detection
  // Only check if we have had at least one breath
  if (last_inhale_time > 0 && (millis() - last_inhale_time > 20000)) { // 20-second threshold
    if (!apnea_alert_active) {
      Serial.println(F("ALERT: Apnea detected! Entering emergency ventilation mode."));
      apnea_alert_active = true;
    }
  }

  // Respiratory Rate Check
  // Check only if we have a valid rate
  if (respiratory_rate > 0) {
    if (respiratory_rate < 8) {
      Serial.println(F("ALERT: Respiratory rate is too low!"));
    } else if (respiratory_rate > 35) {
      Serial.println(F("ALERT: Respiratory rate is too high!"));
    }
  }
}
