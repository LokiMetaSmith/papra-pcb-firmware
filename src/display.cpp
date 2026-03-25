#include "display.h"
#include "menu.h" // For menu states
#include "main.h" // For getPressure()

// Timer for display refresh
static unsigned long last_display_update = 0;
const unsigned long display_update_interval = 200; // 5Hz

// --- Private functions for this module ---

void drawHomeScreen() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);

  display.print(F("Press: ")); display.print((int)getPressure()); display.println(F(" Pa"));
  display.print(F("Set:   ")); display.print((int)pid_setpoint);
  if (pressure_compensation != 0.0) {
    display.print(F(" ("));
    if (pressure_compensation > 0) display.print(F("+"));
    display.print((int)pressure_compensation);
    display.print(F(")"));
  }
  display.println(F(" Pa"));
  display.print(F("Fan:   ")); display.print(fanRPM); display.println(F(" RPM"));
  display.print(F("State: ")); display.println(currentBreathState == STATE_INHALE ? "Inhale" : "Exhale");
  display.print(F("Mute:  ")); display.println(alerts_muted ? "ON" : "OFF");

  if (alert_active) {
    display.println(F("ALERT ACTIVE"));
  } else {
    display.println(F("Status: OK"));
  }
}

void drawMenu() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);

  switch (current_menu_state) {
    case STATE_MAIN_MENU:
      display.println(F("Main Menu"));
      display.println(F("---------"));
      display.print(selected_menu_item == 0 ? ">" : " "); display.println(F(" Set Kp"));
      display.print(selected_menu_item == 1 ? ">" : " "); display.println(F(" Set Ki"));
      display.print(selected_menu_item == 2 ? ">" : " "); display.println(F(" Set Kd"));
      break;
    case STATE_EDIT_KP:
      display.println(F("Set Kp (Proportional)"));
      display.println(F("---------------------"));
      display.print(F("> ")); display.println(Kp);
      break;
    case STATE_EDIT_KI:
      display.println(F("Set Ki (Integral)"));
      display.println(F("-----------------"));
      display.print(F("> ")); display.println(Ki);
      break;
    case STATE_EDIT_KD:
      display.println(F("Set Kd (Derivative)"));
      display.println(F("-------------------"));
      display.print(F("> ")); display.println(Kd);
      break;
    default:
      // Should not happen
      break;
  }
}


// --- Public functions ---

void setupDisplay() {
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    return;
  }
  display.display();
  delay(1000);
  display.clearDisplay();
  display.println(F("PAPRA System Initialized"));
  display.display();
  delay(1000);
}

void updateDisplay() {
  if (millis() - last_display_update < display_update_interval) {
    return;
  }
  last_display_update = millis();

  if (current_menu_state == STATE_HOME_SCREEN) {
    drawHomeScreen();
  } else {
    drawMenu();
  }

  display.display();
}
