#include "user_input.h"
#include "globals.h"
#include "menu.h"
#include <Encoder.h>

// Create Encoder object
Encoder rotary(rotary_A_pin, rotary_B_pin);

// Variables for button debouncing
static long last_button_press_time = 0;
static bool last_button_state = HIGH; // HIGH because of INPUT_PULLUP
const long debounce_duration = 50; // 50 milliseconds

// Variables for tracking encoder position
static long last_encoder_pos = 0;

MenuEvent checkUserInput() {
  // --- Read Encoder Rotation ---
  long new_encoder_pos = rotary.read() / 4; // Divide by 4 for full-step resolution
  if (new_encoder_pos != last_encoder_pos) {
    MenuEvent event = (new_encoder_pos > last_encoder_pos) ? EVENT_NAV_UP : EVENT_NAV_DOWN;
    last_encoder_pos = new_encoder_pos;
    return event;
  }

  // --- Read Encoder Button Press ---
  bool current_button_state = digitalRead(rotary_SW_pin);
  if (current_button_state == LOW && last_button_state == HIGH && (millis() - last_button_press_time) > debounce_duration) {
    last_button_press_time = millis();
    last_button_state = current_button_state; // Update state after processing press
    return EVENT_NAV_SELECT;
  }
  last_button_state = current_button_state;

  return EVENT_NONE; // No event detected
}
