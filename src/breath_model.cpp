#include "breath_model.h"
#include "globals.h"

void updateBreathModel(double raw_pressure) {
  // --- 1. Filter the raw pressure signal ---
  pressure_filter_buffer[pressure_filter_index] = raw_pressure;
  pressure_filter_index = (pressure_filter_index + 1) % 5;
  double filtered_pressure = 0;
  for (int i = 0; i < 5; i++) {
    filtered_pressure += pressure_filter_buffer[i];
  }
  filtered_pressure /= 5.0;

  // --- 2. Use filtered pressure for slope calculation ---
  pressure_history[pressure_history_index] = filtered_pressure;
  int oldest_index = (pressure_history_index + 1) % 10;
  double slope = pressure_history[pressure_history_index] - pressure_history[oldest_index];
  pressure_history_index = (pressure_history_index + 1) % 10;

  // --- 3. Update state machine and track peaks ---
  double inhale_trigger_slope;
  double exhale_trigger_slope;

  if (is_in_learning_phase) {
    // During the learning phase, use fixed, gentle thresholds
    inhale_trigger_slope = 2.0;
    exhale_trigger_slope = -2.0;
  } else {
    // After learning, use adaptive thresholds based on user's effort
    const float trigger_sensitivity = 0.5; // 50% of the average peak
    inhale_trigger_slope = avg_peak_inhale_slope * trigger_sensitivity;
    exhale_trigger_slope = avg_peak_exhale_slope * trigger_sensitivity;
  }

  if (currentBreathState == STATE_INHALE) {
    // Track the peak inhalation slope for this breath
    if (slope > peak_inhale_slope) {
      peak_inhale_slope = slope;
    }

    // If we are inhaling, look for a negative slope to switch to exhale
    if (slope < exhale_trigger_slope) {
      currentBreathState = STATE_EXHALE;
      peak_exhale_slope = 0;
    }
  } else { // STATE_EXHALE
    // Track the peak exhalation slope for this breath
    if (slope < peak_exhale_slope) {
      peak_exhale_slope = slope;
    }

    // If we are exhaling, look for a positive slope to switch to inhale
    if (slope > inhale_trigger_slope) {
      // --- End of a full breath cycle ---
      currentBreathState = STATE_INHALE;

      // Update the running average of the peak slopes
      if (peak_inhale_slope > 0) {
        avg_peak_inhale_slope = (0.8 * avg_peak_inhale_slope) + (0.2 * peak_inhale_slope);
      }
      if (peak_exhale_slope < 0) {
        avg_peak_exhale_slope = (0.8 * avg_peak_exhale_slope) + (0.2 * peak_exhale_slope);
      }
      peak_inhale_slope = 0;

      // --- Calculate Respiratory Rate ---
      unsigned long now = millis();
      if (last_inhale_time > 0) {
        double interval_s = (double)(now - last_inhale_time) / 1000.0;
        respiratory_rate = 60.0 / interval_s;
        if (is_in_learning_phase) {
          rr_accumulator += respiratory_rate;
          rr_sample_count++;
        }
      }
      last_inhale_time = now;

      if (apnea_alert_active) {
        Serial.println(F("Normal breathing resumed. Exiting emergency mode."));
        apnea_alert_active = false;
      }
    }
  }
}
