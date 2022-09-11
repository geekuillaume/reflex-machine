export interface GameState {
  state: "IDLE" | "WAITING_FOR_FIRST_PRESS" | "RUNNING" | "FINISHED";
  missed: number;
  pressed: number;
  delaySum: number;
}

export interface GameStateMessage extends GameState {
  type: "gameState";
}

export type ReceivedMessage = GameStateMessage;

export interface StopGameMessage {
  type: "stopGame";
}

export type SentMessage = StopGameMessage;
