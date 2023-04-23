#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_random.h"
#include "esp_intr_alloc.h"

#include "./main.hpp"

static xQueueHandle gpioEvtQueue = NULL;

typedef struct {
  gpio_num_t buttonPin;
  gpio_num_t ledPin;
  unsigned long ledTurnedOnAt;
  TickType_t lastPressedAtTick;
  bool debouncing; // already got a "missed" press interrupt, wait for debounce before
} reflex_button;

reflex_button buttons[] = {
  { .buttonPin = GPIO_NUM_13, .ledPin = GPIO_NUM_15, .ledTurnedOnAt = 0, .lastPressedAtTick = 0, .debouncing = false },
  { .buttonPin = GPIO_NUM_12, .ledPin = GPIO_NUM_2,  .ledTurnedOnAt = 0, .lastPressedAtTick = 0, .debouncing = false },
  { .buttonPin = GPIO_NUM_14, .ledPin = GPIO_NUM_0,  .ledTurnedOnAt = 0, .lastPressedAtTick = 0, .debouncing = false },
  { .buttonPin = GPIO_NUM_27, .ledPin = GPIO_NUM_4,  .ledTurnedOnAt = 0, .lastPressedAtTick = 0, .debouncing = false },
  { .buttonPin = GPIO_NUM_26, .ledPin = GPIO_NUM_16, .ledTurnedOnAt = 0, .lastPressedAtTick = 0, .debouncing = false },
  { .buttonPin = GPIO_NUM_25, .ledPin = GPIO_NUM_17, .ledTurnedOnAt = 0, .lastPressedAtTick = 0, .debouncing = false },
  { .buttonPin = GPIO_NUM_33, .ledPin = GPIO_NUM_5,  .ledTurnedOnAt = 0, .lastPressedAtTick = 0, .debouncing = false },
  { .buttonPin = GPIO_NUM_32, .ledPin = GPIO_NUM_18, .ledTurnedOnAt = 0, .lastPressedAtTick = 0, .debouncing = false },
  { .buttonPin = GPIO_NUM_22, .ledPin = GPIO_NUM_19, .ledTurnedOnAt = 0, .lastPressedAtTick = 0, .debouncing = false },
  { .buttonPin = GPIO_NUM_23, .ledPin = GPIO_NUM_21, .ledTurnedOnAt = 0, .lastPressedAtTick = 0, .debouncing = false },
};

#define BUTTONS_COUNT (sizeof(buttons) / sizeof(buttons[0]))

GAME_STATE gameState = IDLE;
unsigned int gameButtonsPressed = 0;
unsigned int gameButtonsMissed = 0;
unsigned long gameStartedAt = 0;
unsigned long gameLastActionTime = 0;

static const char* TAG = "ReflexBoardGame";

void turnOffButton(int buttonIndex) {
  buttons[buttonIndex].ledTurnedOnAt = 0;
  gpio_set_level(buttons[buttonIndex].ledPin, 0);
}

void turnOffAllButtons() {
  for (int i = 0; i < BUTTONS_COUNT; i++) {
    turnOffButton(i);
  }
}

void turnOnButton(int buttonIndex) {
  buttons[buttonIndex].ledTurnedOnAt = millis();
  gpio_set_level(buttons[buttonIndex].ledPin, 1);
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
  blinkAllButtonsTaskHandle = NULL;
  vTaskDelete(NULL);
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

// Button needs to have been low for at least 30ms before any falling edge is registered as a click
#define TICK_DEBOUNCE 30

void IRAM_ATTR gpio_isr_handler(void* arg)
{
  uint32_t buttonIdx = (uint32_t) arg;
  int64_t delayFromLastSeenHigh = xTaskGetTickCountFromISR() - buttons[buttonIdx].lastPressedAtTick;

  if (delayFromLastSeenHigh > TICK_DEBOUNCE) {
    xQueueSendFromISR(gpioEvtQueue, &buttonIdx, NULL);
  }
  buttons[buttonIdx].lastPressedAtTick = xTaskGetTickCountFromISR();
}

// Continuously update the lastPressedAtTick prop of the button to make sure a button kept pressed for a few seconds
// then released won't trigger a button press event
void monitorPinLastPressed(void *param) {
  for(;;) {
    for (uint32_t i = 0; i < BUTTONS_COUNT; i++) {
      if (gpio_get_level(buttons[i].buttonPin) == 0) {
        buttons[i].lastPressedAtTick = xTaskGetTickCount();
      }
    }
    vTaskDelay(1);
  }
}

void handleButtonPressedTask(void *param) {
  uint32_t buttonIdx;

  for (;;) {
    if(xQueueReceive(gpioEvtQueue, &buttonIdx, portMAX_DELAY)) {
      ESP_LOGW(TAG, "Got pressed button %u", buttonIdx);

      if (gameState == RUNNING) {
        if (buttons[buttonIdx].ledTurnedOnAt) {
          gameButtonsPressed++;
          turnOffButton(buttonIdx);
        } else {
          gameButtonsMissed++;
        }
        gameLastActionTime = millis();
        broadcastGameState();

        if (gameButtonsPressed >= GAME_BUTTONS_DURATION_PRESSED) {
          ESP_LOGW(TAG, "was last button, game is finished");
          gameState = FINISHED;
          broadcastGameState();
          blinkAllButtons();
          continue;
        }

        int onButtonsCount = countOnButtons();
        if (
          onButtonsCount >= GAME_BUTTONS_ON_IN_PARALLEL ||
          gameButtonsPressed + onButtonsCount >= GAME_BUTTONS_DURATION_PRESSED
        ) {
          ESP_LOGW(TAG, "remaining buttons are already light up, nothing to do");
          continue;
        }

        // need to light up a new button
        // turn on new random button which is not one already on and not the one just pressed
        uint32_t newButtonId;
        do {
          newButtonId = esp_random() % BUTTONS_COUNT;
        } while (newButtonId == buttonIdx || buttons[newButtonId].ledTurnedOnAt != 0);

        ESP_LOGW(TAG, "turning on new button %u", newButtonId);
        turnOnButton(newButtonId);
        continue;
      }

      if (gameState == IDLE || gameState == FINISHED) {
        stopBlinkAllButtons();
        turnOnAllButtons(); // reset ledTurnOnAt for every button to reset delay of other buttons
        gameState = WAITING_FOR_FIRST_PRESS;
        ESP_LOGW(TAG, "Waiting for first press");
        broadcastGameState();
        continue;
      }

      if (gameState == WAITING_FOR_FIRST_PRESS) {
        turnOnAllButtons(); // reset ledTurnOnAt for every button to reset delay of other buttons
        gameState = RUNNING;
        gameButtonsPressed = 0;
        gameButtonsMissed = 0;
        gameStartedAt = millis();
        ESP_LOGW(TAG, "Starting game");
        turnOffButton(buttonIdx);
        broadcastGameState();
        continue;
      }
    }
  }
}

void setupGame() {
  gpio_config_t inputIoConf;
  gpio_config_t ledIoConf;

  gpioEvtQueue = xQueueCreate(10, sizeof(uint32_t));

  // sending interrupt when button is high to always run the interrupt while it's pressed
  // and so keep the latest timestamp at which it was pressed. This is used for debouncing
  // to make sure we only register a press if the button was not pressed for at least X ms
  inputIoConf.intr_type = GPIO_INTR_ANYEDGE;
  inputIoConf.pin_bit_mask = 0;
  inputIoConf.mode = GPIO_MODE_INPUT;
  inputIoConf.pull_down_en = GPIO_PULLDOWN_DISABLE;
  inputIoConf.pull_up_en = GPIO_PULLUP_ENABLE;

  ledIoConf.intr_type = GPIO_INTR_DISABLE;
  ledIoConf.mode = GPIO_MODE_OUTPUT;
  ledIoConf.pin_bit_mask = 0;
  ledIoConf.pull_down_en = GPIO_PULLDOWN_DISABLE;
  ledIoConf.pull_up_en = GPIO_PULLUP_DISABLE;

  for (int i = 0; i < BUTTONS_COUNT; i++) {
    inputIoConf.pin_bit_mask |= 1ULL << buttons[i].buttonPin;
    ledIoConf.pin_bit_mask |= 1ULL << buttons[i].ledPin;

    buttons[i].ledTurnedOnAt = 0;
    buttons[i].lastPressedAtTick = 0;
  }
  gpio_config(&inputIoConf);
  gpio_config(&ledIoConf);

  blinkAllButtons();

  //install gpio isr service
  gpio_install_isr_service(0);

  for (int i = 0; i < BUTTONS_COUNT; i++) {
    gpio_isr_handler_add(buttons[i].buttonPin, gpio_isr_handler, (void*) i);
  }

  xTaskCreatePinnedToCore(
    handleButtonPressedTask,
    "handleButtonPressed",
    2048,
    NULL,
    2,
    NULL,
    1
  );

  xTaskCreatePinnedToCore(
    monitorPinLastPressed,
    "monitorPinLastPressed",
    2048,
    NULL,
    2,
    NULL,
    1
  );
}
