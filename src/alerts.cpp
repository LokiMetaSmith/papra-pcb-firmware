// Implementation of alert system functions
#include "globals.h"
#include "alerts.h"

#include "alerts.h"
#include "globals.h"

void triggerVisualAlert() {
  // Simple visual alert: flash all LEDs rapidly
  static unsigned long last_led_toggle = 0;
  if (millis() - last_led_toggle > 150) {
    digitalWrite(led1, !digitalRead(led1));
    digitalWrite(led2, !digitalRead(led2));
    digitalWrite(led3, !digitalRead(led3));
    digitalWrite(led4, !digitalRead(led4));
    last_led_toggle = millis();
  }
}

void triggerAudibleAlert() {
  // Simple audible alert: short beeps
  // This is blocking, but only runs when alerts are not muted.
  if (!alerts_muted) {
    digitalWrite(buzzerPin, HIGH);
    delay(50);
    digitalWrite(buzzerPin, LOW);
    delay(50);
    digitalWrite(buzzerPin, HIGH);
    delay(50);
    digitalWrite(buzzerPin, LOW);
  }
}

void checkAlerts() {
  bool condition_found = false;

  // Apnea Detection
  if (last_inhale_time > 0 && (millis() - last_inhale_time > 20000)) { // 20-second threshold
    if (!apnea_alert_active) {
      Serial.println(F("ALERT: Apnea detected! Entering emergency ventilation mode."));
      apnea_alert_active = true;
    }
    condition_found = true;
  }

  // Respiratory Rate Check
  if (respiratory_rate > 0) {
    if (respiratory_rate < 8 || respiratory_rate > 35) {
      Serial.println(F("ALERT: Abnormal respiratory rate!"));
      condition_found = true;
    }
  }

  alert_active = condition_found;
}
