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
  // Define adaptive slope thresholds for switching states.
  const float trigger_sensitivity = 0.5; // 50% of the average peak
  double inhale_trigger_slope = avg_peak_inhale_slope * trigger_sensitivity;
  double exhale_trigger_slope = avg_peak_exhale_slope * trigger_sensitivity;

  if (currentBreathState == STATE_INHALE) {
    // Track the peak inhalation slope for this breath
    if (slope > peak_inhale_slope) {
      peak_inhale_slope = slope;
    }

    // If we are inhaling, look for a negative slope to switch to exhale
    if (slope < exhale_trigger_slope) {
      currentBreathState = STATE_EXHALE;
      // When we switch to exhale, reset the peak exhale slope for the new phase
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
      // During the learning phase, adapt more quickly (factor of 0.5)
      // Otherwise, use a simple moving average with a factor of 0.2 (i.e., last 5 breaths)
      float learning_rate = learning_phase_active ? 0.5 : 0.2;

      if (peak_inhale_slope > 0) { // Only update if we had a valid peak
        avg_peak_inhale_slope = ((1.0 - learning_rate) * avg_peak_inhale_slope) + (learning_rate * peak_inhale_slope);
      }
      if (peak_exhale_slope < 0) { // Only update if we had a valid peak
        avg_peak_exhale_slope = ((1.0 - learning_rate) * avg_peak_exhale_slope) + (learning_rate * peak_exhale_slope);
      }

      // Reset the peak inhale slope for the new phase
      peak_inhale_slope = 0;

      // --- Calculate Respiratory Rate ---
      unsigned long now = millis();
      if (last_inhale_time > 0) {
        double interval_s = (double)(now - last_inhale_time) / 1000.0;
        respiratory_rate = 60.0 / interval_s;

        // --- Learning Phase for Adaptive Thresholds ---
        if (learning_phase_active) {
          // If we are within the first 60 seconds of learning
          if (now - learning_phase_start_time < 60000) {
            learning_rr_sum += respiratory_rate;
            learning_breath_count++;
          } else {
            // Learning phase is over
            learning_phase_active = false;
            if (learning_breath_count > 0) {
              baseline_respiratory_rate = learning_rr_sum / learning_breath_count;
            } else {
              baseline_respiratory_rate = 15.0; // Fallback default
            }
            Serial.print(F("Learning phase complete. Baseline RR: "));
            Serial.println(baseline_respiratory_rate);
          }
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
