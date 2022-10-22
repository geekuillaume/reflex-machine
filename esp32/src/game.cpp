#include "Button2.h"

#include "./main.hpp"

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

TaskHandle_t blinkAllButtonsTaskHandle = NULL;

#define BLINK_X_TIMES 5
void blinkAllButtonsTask(void *params) {
  for(int i = 0; i < BLINK_X_TIMES; i++) {
    turnOnAllButtons();
    vTaskDelay(250 / portTICK_PERIOD_MS);
    turnOffAllButtons();
    vTaskDelay(250 / portTICK_PERIOD_MS);
  }
  vTaskDelete(NULL);
  blinkAllButtonsTaskHandle = NULL;
}

void blinkAllButtons() {
  xTaskCreate(blinkAllButtonsTask, "blinkAllButtons", 2048, NULL, 5, &blinkAllButtonsTaskHandle);
}

void stopBlinkAllButtons() {
  if (blinkAllButtonsTaskHandle) {
    vTaskDelete(blinkAllButtonsTaskHandle);
    blinkAllButtonsTaskHandle = NULL;
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
  gameState = IDLE;
  stopBlinkAllButtons();
  turnOffAllButtons();
  broadcastGameState();

  return true;
}

bool flashAllButtons(void *) {
  bool wasOn = buttons[0].ledTurnedOnAt != 0;

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
    blinkAllButtons();
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

  blinkAllButtons();

  xTaskCreatePinnedToCore(
    loopGame,
    "loop game",
    2048,
    NULL,
    1,
    NULL,
    1
  );
}

uint32_t lastLoopMillis2 = millis();
uint32_t lastLoopPrinted2 = 0;

void loopGame(void *params) {
  for (;;) {
    if (lastLoopPrinted2 > 1000) {
      debugA("1000 loops took %d ms", millis() - lastLoopMillis2);
      lastLoopMillis2 = millis();
      lastLoopPrinted2 = 0;
    }
    lastLoopPrinted2++;

    for (int i = 0; i < BUTTONS_COUNT; i++) {
      buttons[i].button.loop();
    }
    vTaskDelay(0);
  }
}
