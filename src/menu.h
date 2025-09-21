#ifndef MENU_H
#define MENU_H

#include "globals.h"

// Define the states of the menu
enum MenuState {
  STATE_HOME_SCREEN,
  STATE_MAIN_MENU,
  STATE_EDIT_KP,
  STATE_EDIT_KI,
  STATE_EDIT_KD
};

// Define events that can be passed to the menu system
enum MenuEvent {
  EVENT_NONE,
  EVENT_NAV_UP,
  EVENT_NAV_DOWN,
  EVENT_NAV_SELECT
};

// Function prototypes
void setupMenu();
void processMenuEvent(MenuEvent event);

#endif // MENU_H
