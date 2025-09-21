// Implementation of breath model functions
#include "globals.h"
#include "breath_model.h"

void updateBreathModel(double current_pressure) {
  // Store current pressure in history (circular buffer)
  pressure_history[pressure_history_index] = current_pressure;

  // Get the oldest sample's index
  int oldest_index = (pressure_history_index + 1) % 10;

  // Calculate pressure slope. This is a very simple approximation.
  double slope = pressure_history[pressure_history_index] - pressure_history[oldest_index];

#if 0
  Serial.print("Slope: "); Serial.println(slope);
#endif

  pressure_history_index = (pressure_history_index + 1) % 10;

  // Define slope thresholds for switching states. These will need tuning.
  double inhale_trigger_slope = 1.5;
  double exhale_trigger_slope = -1.5;

  if (currentBreathState == STATE_INHALE) {
    // If we are inhaling, look for a negative slope to switch to exhale
    if (slope < exhale_trigger_slope) {
      currentBreathState = STATE_EXHALE;
    }
  } else { // STATE_EXHALE
    // If we are exhaling, look for a positive slope to switch to inhale
    if (slope > inhale_trigger_slope) {
      currentBreathState = STATE_INHALE;
      unsigned long now = millis();
      if (last_inhale_time > 0) {
        // Calculate breath-to-breath interval in seconds
        double interval_s = (double)(now - last_inhale_time) / 1000.0;
        // Convert to breaths per minute
        respiratory_rate = 60.0 / interval_s;
      }
      last_inhale_time = now;

      // If we are recovering from an apnea event, reset the flag.
      if (apnea_alert_active) {
        Serial.println(F("Normal breathing resumed. Exiting emergency mode."));
        apnea_alert_active = false;
      }
    }
  }
}
