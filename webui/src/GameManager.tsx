import { useEffect, useRef, useState } from "react";
import { GAME_DURATION_PRESSES } from "./constants";
import { computeGameScore } from "./lib/lib";
import { ReflexBoard } from "./lib/ReflexBoard";
import { GameState } from "./lib/types";
import { RegisterHighScore } from "./RegisterHighScore";

const testscore = Math.random() * 100000;

export const GameManager = ({ reflexBoard }: { reflexBoard: ReflexBoard }) => {
  const [gameState, setGameState] = useState<GameState | null>({
    startedMsAgo: 34670,
    missed: 2,
    pressed: 90,
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
        <p className="LetsGo">Let's go !</p>
      </div>
    );
  }
  let missColor = "#f3f3f3";
  if (gameState.missed > 0) {
    missColor = "#ffcd3c";
  }
  if (gameState.missed > 3) {
    missColor = "#ff9234";
  }
  if (gameState.missed > 6) {
    missColor = "#d92027";
  }

  return (
    <div className="gameManager">
      <p>Game started {(gameState.startedMsAgo / 1000).toFixed(2)}s ago </p>

      <p>
        Lights pressed: {gameState.pressed} / {GAME_DURATION_PRESSES}
      </p>
      <p>
        Miss count :{" "}
        <span style={{ color: missColor }}>{gameState.missed}</span>
      </p>
      <p>Score: {(computeGameScore(gameState) / 1000).toFixed(2)}s</p>
    </div>
  );
};
