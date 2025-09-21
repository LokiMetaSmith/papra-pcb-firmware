#include "menu.h"
#include "eeprom_config.h" // For saveConfig()

// --- Menu State & Navigation Variables ---
MenuState current_menu_state = STATE_HOME_SCREEN;
int selected_menu_item = 0;
const int MAIN_MENU_ITEM_COUNT = 3;

void setupMenu() {
  // Nothing to do here for now
}

void processMenuEvent(MenuEvent event) {
  switch (current_menu_state) {
    case STATE_HOME_SCREEN:
      if (event == EVENT_NAV_SELECT) {
        // Enter the main menu from the home screen
        current_menu_state = STATE_MAIN_MENU;
        selected_menu_item = 0;
      }
      break;

    case STATE_MAIN_MENU:
      if (event == EVENT_NAV_UP) {
        selected_menu_item = (selected_menu_item + 1) % MAIN_MENU_ITEM_COUNT;
      } else if (event == EVENT_NAV_DOWN) {
        selected_menu_item = (selected_menu_item - 1 + MAIN_MENU_ITEM_COUNT) % MAIN_MENU_ITEM_COUNT;
      } else if (event == EVENT_NAV_SELECT) {
        // Enter the edit state for the selected item
        if (selected_menu_item == 0) current_menu_state = STATE_EDIT_KP;
        else if (selected_menu_item == 1) current_menu_state = STATE_EDIT_KI;
        else if (selected_menu_item == 2) current_menu_state = STATE_EDIT_KD;
      }
      break;

    case STATE_EDIT_KP:
      if (event == EVENT_NAV_UP) Kp += 0.1;
      else if (event == EVENT_NAV_DOWN) Kp -= 0.1;
      else if (event == EVENT_NAV_SELECT) {
        saveConfig(); // Save the new value
        current_menu_state = STATE_MAIN_MENU; // Return to menu
      }
      break;

    case STATE_EDIT_KI:
      if (event == EVENT_NAV_UP) Ki += 0.1;
      else if (event == EVENT_NAV_DOWN) Ki -= 0.1;
      else if (event == EVENT_NAV_SELECT) {
        saveConfig();
        current_menu_state = STATE_MAIN_MENU;
      }
      break;

    case STATE_EDIT_KD:
      if (event == EVENT_NAV_UP) Kd += 0.1;
      else if (event == EVENT_NAV_DOWN) Kd -= 0.1;
      else if (event == EVENT_NAV_SELECT) {
        saveConfig();
        current_menu_state = STATE_MAIN_MENU;
      }
      break;
  }
}
