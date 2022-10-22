import EventEmitter from "eventemitter3";
import { GameState, ReceivedMessage, SentMessage } from "./types";

export class ReflexBoard extends EventEmitter<{
  connected: () => void;
  disconnected: () => void;
  gameState: (state: GameState) => void;
}> {
  websocket: WebSocket | null;
  onStateChange: () => any = () => {};

  constructor() {
    super();
    this.websocket = null;
  }

  connect(host: string) {
    if (this.websocket) {
      return;
    }
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
      if (message.type === "gameState") {
        this.emit("gameState", message);
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

  sendMessage(message: SentMessage) {
    if (!this.websocket) {
      return;
    }
    try {
      this.websocket.send(JSON.stringify(message));
    } catch (e) {
      console.error("Error while sending message", e);
    }
  }

  stopGame() {
    this.sendMessage({
      type: "stopGame",
    });
  }
}
