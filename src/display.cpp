#include "display.h"

// Timer for display refresh
static unsigned long last_display_update = 0;
const unsigned long display_update_interval = 250; // 4Hz

void setupDisplay() {
  // Initialize the OLED display
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    // Don't halt, the device can still function without a display.
    return;
  }

  display.display();
  delay(1000);

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);
  display.println(F("PAPRA System Initialized"));
  display.display();
  delay(1000);
}

void updateDisplay() {
  // Limit the refresh rate
  if (millis() - last_display_update < display_update_interval) {
    return;
  }
  last_display_update = millis();

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);

  // Line 1: Pressure
  display.print(F("Press: "));
  display.print((int)getPressure());
  display.print(F(" Pa"));

  // Line 2: Setpoint
  display.setCursor(0, 10);
  display.print(F("Set:   "));
  display.print((int)pid_setpoint);
  display.print(F(" Pa"));

  // Line 3: Fan RPM
  display.setCursor(0, 20);
  display.print(F("Fan:   "));
  display.print(fanRPM);
  display.print(F(" RPM"));

  // Line 4: Breathing State
  display.setCursor(0, 30);
  display.print(F("State: "));
  display.print(currentBreathState == STATE_INHALE ? "Inhale" : "Exhale");

  // Line 5: Mute Status
  display.setCursor(0, 40);
  display.print(F("Mute:  "));
  display.print(alerts_muted ? "ON" : "OFF");

  // Line 6: Alert Status
  display.setCursor(0, 50);
  if (alert_active) {
    display.print(F("ALERT ACTIVE"));
  } else {
    display.print(F("Status: OK"));
  }

  display.display();
}
