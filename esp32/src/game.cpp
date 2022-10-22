#include "Button2.h"
#include <arduino-timer.h>

#include "./game.hpp"
#include "./preferences.hpp"
#include "./webserver.hpp"

typedef struct {
  int buttonPin;
  int ledPin;
  unsigned long ledTurnedOnAt;
  Button2 button;
} reflex_button;

#define BUTTONS_COUNT 10
reflex_button buttons[] = {
  { .buttonPin = 13, .ledPin = 15 },
  { .buttonPin = 12, .ledPin = 2 },
  { .buttonPin = 14, .ledPin = 0 },
  { .buttonPin = 27, .ledPin = 4 },
  { .buttonPin = 26, .ledPin = 16 },
  { .buttonPin = 25, .ledPin = 17 },
  { .buttonPin = 33, .ledPin = 5 },
  { .buttonPin = 32, .ledPin = 18 },
  { .buttonPin = 22, .ledPin = 19 },
  { .buttonPin = 23, .ledPin = 21 },
};

GAME_STATE gameState = IDLE;
unsigned int gameButtonsPressed = 0;
unsigned int gameButtonsMissed = 0;
unsigned long gameStartedAt = 0;
unsigned long gameLastActionTime = 0;


auto beforeIdleTimer = timer_create_default(); // timer to re-start idle state after finished
auto afterFinishedLightFlashTimer = timer_create_default(); // timer to flash lights after finished


void turnOffButton(int buttonIndex) {
  buttons[buttonIndex].ledTurnedOnAt = 0;
  digitalWrite(buttons[buttonIndex].ledPin, LOW);
}

void turnOffAllButtons() {
  for (int i = 0; i < BUTTONS_COUNT; i++) {
    turnOffButton(i);
  }
}

void turnOnButton(int buttonIndex) {
  buttons[buttonIndex].ledTurnedOnAt = millis();
  digitalWrite(buttons[buttonIndex].ledPin, HIGH);
}

void turnOnAllButtons() {
  for (int i = 0; i < BUTTONS_COUNT; i++) {
    turnOnButton(i);
  }
}

void stopGame() {
  gameState = FINISHED;
  turnOffAllButtons();
}

int countOnButtons() {
  int count = 0;
  for (int i = 0; i < BUTTONS_COUNT; i++) {
    if (buttons[i].ledTurnedOnAt != 0) {
      count++;
    }
  }

  return count;
}

bool goToIdle(void *) {
  Serial.println("Flashing buttons");

  gameState = IDLE;
  afterFinishedLightFlashTimer.cancel();
  turnOffAllButtons();
  broadcastGameState();

  return true;
}

bool flashAllButtons(void *) {
  bool wasOn = buttons[0].ledTurnedOnAt != 0;

  Serial.println("Flashing buttons");
  if (wasOn) {
    turnOffAllButtons();
  } else {
    turnOnAllButtons();
  }

  return true;
}

void onButtonPressed(Button2& btn) {
  if (gameState == FINISHED) {
    return;
  }

  if (gameState == IDLE) {
    turnOnAllButtons(); // reset ledTurnOnAt for every button to reset delay of other buttons
    gameState = WAITING_FOR_FIRST_PRESS;
    broadcastGameState();
    return;
  }

  if (gameState == WAITING_FOR_FIRST_PRESS) {
    turnOnAllButtons(); // reset ledTurnOnAt for every button to reset delay of other buttons
    gameState = RUNNING;
    gameButtonsPressed = 0;
    gameButtonsMissed = 0;
    gameStartedAt = millis();
  }

  if (buttons[btn.getID()].ledTurnedOnAt == 0) {
    gameButtonsMissed++;
  } else {
    gameButtonsPressed++;
    turnOffButton(btn.getID());
  }
  gameLastActionTime = millis();

  int onButtonsCount = countOnButtons();

  if (gameButtonsPressed >= GAME_BUTTONS_DURATION_PRESSED) {
    gameState = FINISHED;
    beforeIdleTimer.in(5000, goToIdle);
    afterFinishedLightFlashTimer.every(250, flashAllButtons);
  } else if (
    onButtonsCount < GAME_BUTTONS_ON_IN_PARALLEL &&
    gameButtonsPressed + onButtonsCount < GAME_BUTTONS_DURATION_PRESSED
  ) {
    // turn on new random button which is not one already on and not the one just pressed
    int newButtonId;
    do {
      newButtonId = floor(random(0, BUTTONS_COUNT));
    } while (newButtonId == btn.getID() || buttons[newButtonId].ledTurnedOnAt != 0);

    turnOnButton(newButtonId);
  }

  broadcastGameState();
}

void setupGame() {
  for (int i = 0; i < BUTTONS_COUNT; i++) {
    buttons[i].button.begin(buttons[i].buttonPin);
    buttons[i].button.setID(i);
    buttons[i].button.setPressedHandler(onButtonPressed);

    pinMode(buttons[i].ledPin, OUTPUT);
    digitalWrite(buttons[i].ledPin, LOW);

    buttons[i].ledTurnedOnAt = 0;
  }

  beforeIdleTimer.in(5000, goToIdle);
  afterFinishedLightFlashTimer.every(250, flashAllButtons);
}

void loopGame() {
  afterFinishedLightFlashTimer.tick();
  beforeIdleTimer.tick();
  for (int i = 0; i < BUTTONS_COUNT; i++) {
    buttons[i].button.loop();
  }
}
