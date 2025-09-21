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
- [x] **Improve Alerting:**
    - [x] Re-introduce the buzzer by migrating to a larger MCU.
    - [x] Implement a visual and audible alert system (e.g., flashing LEDs and sounding the buzzer).
- [x] **Add a User Interface:**
    - [x] Add support for an I2C OLED display to show real-time data (pressure, RPM, state, etc.).
    - [x] Implement a configuration menu using the display and a rotary encoder.

## 3. Code & Testing
- [x] **Hardware Documentation & Simulation:**
    - [x] Create `wireviz` source file (`wiring.yml`) for hardware documentation.
    - [x] Create Wokwi simulation configuration (`wokwi.toml`, `diagram.json`).
- [ ] **Advanced Breath Modeling:**
    - [ ] Improve the breath detection algorithm for higher accuracy and robustness.
