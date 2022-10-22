
#include <Preferences.h>

#include "./game.hpp"
#include "./webserver.hpp"
#include "./preferences.hpp"

void setup(){
  // Serial port for debugging purposes
  Serial.begin(115200);
  Serial.println("Starting");

  setupPreferences();
  setupWebserver();
  setupGame();
}

void loop() {
  loopWebserver();
  loopGame();
}
