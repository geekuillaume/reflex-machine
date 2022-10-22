#pragma once

typedef enum  {
  IDLE,
  WAITING_FOR_FIRST_PRESS,
  RUNNING,
  FINISHED
} GAME_STATE;

extern GAME_STATE gameState;

extern unsigned int gameButtonsPressed;
extern unsigned int gameButtonsMissed;
extern unsigned long gameStartedAt;
extern unsigned long gameLastActionTime;

void stopGame();
void setupGame();
void loopGame();

