import EventEmitter from "eventemitter3";
import { ReceivedMessage, SentMessage } from "../types";

export class ReflexBoard extends EventEmitter<{
  connected: () => void;
  disconnected: () => void;
  pressedButton: (buttonId: number, delay: number) => void;
  missedButton: (buttonId: number) => void;
}> {
  websocket: WebSocket | null;
  onStateChange: () => any = () => {};

  constructor() {
    super();
    this.websocket = null;
  }

  connect(host: string) {
    const websocket = new WebSocket(`ws://${host}/ws`);
    websocket.addEventListener("open", () => {
      this.websocket = websocket;
      console.log("Connected");
      this.emit("connected");
      this.onStateChange();
    });
    websocket.addEventListener("close", () => {
      console.log("Disconnected");
      this.websocket = null;
      this.emit("disconnected");
      this.onStateChange();
    });
    websocket.addEventListener("message", async (e) => {
      const message = JSON.parse(e.data) as ReceivedMessage;
      console.log("Received:", message);
      if (message.type === "pressed") {
        this.emit("pressedButton", message.buttonId, message.delay);
      } else if (message.type === "missed") {
        this.emit("missedButton", message.buttonId);
      }
    });
  }

  disconnect() {
    if (this.websocket) {
      this.websocket.close();
      this.websocket = null;
    }
    this.onStateChange();
  }

  lightUpButtons(buttonsId: number[]) {
    if (!this.websocket) {
      return;
    }
    const message: SentMessage = {
      type: "lightsOn",
      buttonsId,
    };

    this.websocket.send(JSON.stringify(message));
  }

  lightOffButtons(buttonsId: number[]) {
    if (!this.websocket) {
      return;
    }
    const message: SentMessage = {
      type: "lightsOff",
      buttonsId,
    };

    this.websocket.send(JSON.stringify(message));
  }
}
