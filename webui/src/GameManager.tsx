import { useRef, useState } from "react";
import { BUTTONS_COUNT } from "./constants";
import { ReflexBoard } from "./lib/ReflexBoard";
import { ReflexGame } from "./lib/ReflexGame";
import { useRerender } from "./lib/useRerender";

export const GameManager = ({ reflexBoard }: { reflexBoard: ReflexBoard }) => {
  const rerender = useRerender();
  const reflexGameRef = useRef<ReflexGame | null>(null);

  const handleStartGame = () => {
    if (reflexGameRef.current) {
      reflexGameRef.current.handleGameFinised();
    }

    reflexGameRef.current = new ReflexGame(reflexBoard);
    reflexGameRef.current.onChange = () => rerender();
    reflexGameRef.current.startGame();
  };

  const handleStopGame = () => {
    reflexGameRef.current?.handleGameFinised();
  };

  if (!reflexBoard.websocket) {
    return <div>Waiting for connection...</div>;
  }

  return (
    <div>
      {!reflexGameRef.current || reflexGameRef.current.state !== "FINISHED"}
      <button
        onClick={handleStartGame}
        disabled={reflexGameRef.current?.state === "RUNNING"}
      >
        Start Game
      </button>
      <button
        onClick={handleStopGame}
        disabled={
          !reflexGameRef.current || reflexGameRef.current.state !== "RUNNING"
        }
      >
        Stop game
      </button>
      <p>Step: {reflexGameRef.current?.stepIdx}</p>
      <p>Score: {reflexGameRef.current?.getScore()}</p>
    </div>
  );
};
