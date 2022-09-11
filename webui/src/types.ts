export interface PressedMessage {
  type: "pressed";
  buttonId: number;
  delay: number;
}

export interface MissedMessage {
  type: "missed";
  buttonId: number;
}

export type ReceivedMessage = PressedMessage | MissedMessage;

export interface LightsOnMessage {
  type: "lightsOn";
  buttonsId: number[];
}

export interface LightsOffMessage {
  type: "lightsOff";
  buttonsId: number[];
}

export type SentMessage = LightsOnMessage | LightsOffMessage;
