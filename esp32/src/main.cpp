#include "./main.hpp"

void setup(){
  // Serial port for debugging purposes
  Serial.begin(115200);
  debugA("Starting Reflex Machine");

  setupPreferences();
  setupWebserver();
  setupGame();
}

void loop() {}
