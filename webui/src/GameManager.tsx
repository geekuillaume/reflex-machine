import { useEffect, useRef, useState } from "react";
import { GAME_DURATION_PRESSES } from "./constants";
import { computeGameScore } from "./lib/lib";
import { ReflexBoard } from "./lib/ReflexBoard";
import { GameState } from "./lib/types";
import { RegisterHighScore } from "./RegisterHighScore";

const testscore = Math.random() * 100000;

export const GameManager = ({ reflexBoard }: { reflexBoard: ReflexBoard }) => {
  const [gameState, setGameState] = useState<GameState | null>({
    delaySum: 35000,
    missed: 5,
    pressed: 30,
    state: "IDLE",
  });
  let idleMessage = "Press any button to start...";

  useEffect(() => {
    const listener = (state: GameState) => {
      setGameState(state);
    };
    reflexBoard.on("gameState", listener);
    return () => {
      reflexBoard.off("gameState", listener);
    };
  });

  // Used to test display without actual game
  // return (
  //   <RegisterHighScore
  //     gameState={{
  //       startedMsAgo: testscore,
  //       missed: 5,
  //       pressed: 100,
  //       state: "FINISHED",
  //     }}
  //   />
  // );

  // if (!reflexBoard.websocket) {
  //   return <div>Waiting for connection...</div>;
  // }

  if (!gameState) {
    return <p>Waiting for state...</p>;
  }

  if (
    gameState.state === "FINISHED" ||
    (gameState.state === "IDLE" && gameState.pressed !== 0)
  ) {
    return (
      <div>
        <RegisterHighScore gameState={gameState} />
      </div>
    );
  }

  if (gameState.state === "IDLE") {
    return (
      <div className="waivy">
        {idleMessage.split("").map((letter, index) => {
          return (
            <span style={{ "--i": index }}>
              {letter == " " ? "\u00A0" : letter}
            </span>
          );
        })}
      </div>
    );
  }

  if (gameState.state === "WAITING_FOR_FIRST_PRESS") {
    return (
      <div>
        <p>Let's go !</p>
      </div>
    );
  }

  return (
    <div className="gameManager">
      <p>State: {gameState.state}</p>
      <p>
        Pressed count: {gameState.pressed} / {GAME_DURATION_PRESSES}
      </p>
      <p>Missed count: {gameState.missed}</p>
      <p>Started ms ago: {gameState.startedMsAgo}</p>
      <p>Score: {(computeGameScore(gameState) / 1000).toFixed(2)}s</p>
    </div>
  );
};
