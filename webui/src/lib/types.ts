export interface GameState {
  state: "IDLE" | "WAITING_FOR_FIRST_PRESS" | "RUNNING" | "FINISHED";
  missed: number;
  pressed: number;
  startedMsAgo: number;
}

export interface GameStateMessage extends GameState {
  type: "gameState";
}

export type ReceivedMessage = GameStateMessage;

export interface StopGameMessage {
  type: "stopGame";
}

export interface PreferencesMessage {
  type: "preferences";
  parallel?: number;
  duration?: number;
}

export type SentMessage = StopGameMessage | PreferencesMessage;
