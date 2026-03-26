#include "eeprom_config.h"

// Define a "magic number" to check if the EEPROM has been initialized
#define EEPROM_MAGIC 0xDEADBEEF

void loadConfig() {
  Config cfg;
  EEPROM.get(0, cfg);

  if (cfg.magic_number == EEPROM_MAGIC) {
    // EEPROM has valid data, load it
    Serial.println(F("Loading configuration from EEPROM..."));
    Kp = cfg.Kp;
    Ki = cfg.Ki;
    Kd = cfg.Kd;
    baseline_impedance = cfg.baseline_impedance;
    apnea_duration_threshold = cfg.apnea_duration_threshold;
    rr_min_threshold = cfg.rr_min_threshold;
    rr_max_threshold = cfg.rr_max_threshold;
  } else {
    // EEPROM is not initialized or data is corrupt
    // Load default values and save them
    Serial.println(F("EEPROM not initialized. Loading default config and saving."));
    // The default values are already in the global variables,
    // so we just need to save them.
    saveConfig();
  }
}

void saveConfig() {
  Config cfg;
  cfg.magic_number = EEPROM_MAGIC;
  cfg.Kp = Kp;
  cfg.Ki = Ki;
  cfg.Kd = Kd;
  cfg.baseline_impedance = baseline_impedance;
  cfg.apnea_duration_threshold = apnea_duration_threshold;
  cfg.rr_min_threshold = rr_min_threshold;
  cfg.rr_max_threshold = rr_max_threshold;

  EEPROM.put(0, cfg);
  Serial.println(F("Configuration saved to EEPROM."));
}
