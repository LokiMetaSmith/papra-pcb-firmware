# Project TODO List

This list tracks potential next steps and improvements for the PAPR firmware project.

## 1. Tuning & Calibration (Requires Hardware)
- [ ] Tune PID controller constants (`Kp`, `Ki`, `Kd`).
- [ ] Tune breath model trigger thresholds for inhale/exhale detection.
- [ ] Tune alert system thresholds (apnea duration, respiratory rate limits).
- [ ] Perform an initial calibration run to establish a baseline for filter health.

## 2. Feature Enhancements
- [x] **Configuration Management:**
    - [x] Add serial commands to set PID constants, pressure setpoints, and alert thresholds.
    - [x] Store these configuration values in EEPROM to make them persistent.
- [ ] **Improve Alerting:**
    - [ ] Implement a visual alert system (e.g., flashing all LEDs in a specific pattern).
- [ ] **Add a User Interface:**
    - [ ] Add support for an I2C OLED display to show real-time data (pressure, RPM, state, etc.).

## 3. Code & Testing
- [ ] **Wokwi Simulation:**
    - [ ] Create the configuration files (`wokwi.toml`, `diagram.json`) to simulate the project.
- [ ] **Advanced Breath Modeling:**
    - [ ] Improve the breath detection algorithm for higher accuracy and robustness.
