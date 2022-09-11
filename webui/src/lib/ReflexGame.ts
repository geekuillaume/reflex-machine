import { BUTTONS_COUNT } from "../constants";
import { ReflexBoard } from "./ReflexBoard";

const generateScenario = () => {
  const steps = [];
  steps.push({
    name: "allOn",
    buttons: [0, 1, 2, 3, 4, 5, 6, 7, 8, 9],
  });

  steps.push({
    name: "oneRandom",
    buttons: new Array(1)
      .fill(0)
      .map(() => Math.floor(Math.random() * BUTTONS_COUNT)),
  });

  steps.push({
    name: "twoRandom",
    buttons: new Array(2)
      .fill(0)
      .map(() => Math.floor(Math.random() * BUTTONS_COUNT)),
  });

  steps.push({
    name: "threeRandom",
    buttons: new Array(3)
      .fill(0)
      .map(() => Math.floor(Math.random() * BUTTONS_COUNT)),
  });

  return steps;
};

export class ReflexGame {
  public scenario = generateScenario();
  public stepIdx = -1;
  public missedCount = 0;
  public pressedDelaySum = 0;
  public onChange: () => any = () => {};
  public state: "IDLE" | "RUNNING" | "FINISHED" = "IDLE";
  public initialDelay = -1;

  constructor(public reflexBoard: ReflexBoard) {}

  startGame() {
    this.state = "RUNNING";
    this.reflexBoard.on("missedButton", this.handleMissed);
    this.reflexBoard.on("pressedButton", this.handlePressed);
    this.reflexBoard.on("disconnected", this.handleGameFinised);
    this.triggerNextStep();
    this.onChange();
  }

  handleMissed = () => {
    this.missedCount++;
    this.onChange();
  };

  handlePressed = (buttonIndex: number, delay: number) => {
    console.log("here");
    if (this.initialDelay === -1) {
      this.initialDelay = delay;
    }
    // Done to trigger game actual start when first button is pressed
    const actualDelay = this.stepIdx === 0 ? delay - this.initialDelay : delay;

    this.pressedDelaySum += actualDelay;
    this.scenario[this.stepIdx].buttons = this.scenario[
      this.stepIdx
    ].buttons.filter((idx) => idx !== buttonIndex);
    console.log(this.scenario, this.stepIdx);
    if (this.scenario[this.stepIdx].buttons.length === 0) {
      this.triggerNextStep();
    }
    this.onChange();
  };

  triggerNextStep() {
    console.log("triggering next step");
    this.stepIdx++;
    if (this.stepIdx >= this.scenario.length) {
      this.handleGameFinised();
      return;
    }
    this.reflexBoard.lightOffButtons(
      new Array(BUTTONS_COUNT).fill(0).map((_, i) => i)
    );
    this.reflexBoard.lightUpButtons(this.scenario[this.stepIdx].buttons);
  }

  handleGameFinised() {
    this.reflexBoard.off("missedButton", this.handleMissed);
    this.reflexBoard.off("pressedButton", this.handlePressed);
    this.reflexBoard.off("disconnected", this.handleGameFinised);
    this.reflexBoard.lightOffButtons(
      new Array(BUTTONS_COUNT).fill(0).map((_, i) => i)
    );
    this.state = "FINISHED";
    this.onChange();
  }

  getScore() {
    return this.pressedDelaySum;
    // return this.
  }
}
