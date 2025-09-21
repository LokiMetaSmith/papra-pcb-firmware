#include "input.h"
#include "globals.h"

// Variables for debouncing the mute button
static unsigned long last_button_press_time = 0;
static bool last_button_state = HIGH; // HIGH because of INPUT_PULLUP
const unsigned long debounce_duration = 50; // 50 milliseconds

void checkMuteButton() {
  bool current_button_state = digitalRead(muteButtonPin);

  // Check for a falling edge (the moment the button is pressed)
  // and ensure enough time has passed since the last press to debounce.
  if (current_button_state == LOW && last_button_state == HIGH && (millis() - last_button_press_time) > debounce_duration) {
    alerts_muted = !alerts_muted; // Toggle the mute state
    last_button_press_time = millis(); // Record the time of this press

    Serial.print(F("Alerts Muted: "));
    Serial.println(alerts_muted ? "Yes" : "No");
  }

  last_button_state = current_button_state;
}
