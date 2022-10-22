#include "./main.hpp"

uint GAME_BUTTONS_ON_IN_PARALLEL = 3;
uint GAME_BUTTONS_DURATION_PRESSED = 15; // Game stop after 100 presses

Preferences preferences;

void setupPreferences() {
  preferences.begin("reflex-machine", false);
  // GAME_BUTTONS_DURATION_PRESSED = preferences.getUInt("duration", 100);
  // GAME_BUTTONS_ON_IN_PARALLEL = preferences.getUInt("parallel", 2);
}
