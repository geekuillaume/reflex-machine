import { useEffect, useState } from "react";
import reactLogo from "./assets/react.svg";
import "./reset.css";
import "./App.css";
import { ConnectionManager } from "./ConnectionManager";
import { BUTTONS_COUNT } from "./constants";
import { GameManager } from "./GameManager";
import { ReflexBoard } from "./lib/ReflexBoard";
import { useRerender } from "./lib/useRerender";
import { ScoreBoard } from "./ScoreBoard";

const reflexBoard = new ReflexBoard();

function App() {
  const rerender = useRerender();

  useEffect(() => {
    reflexBoard.onStateChange = () => {
      rerender();
    };
  }, []);

  return (
    <div className="App">
      <div className="titleContainer">
        <h1 className="title">
          Reflex <br /> Machine
        </h1>
      </div>
      <div className="card">
        <GameManager reflexBoard={reflexBoard} />
        <ScoreBoard />
        <ConnectionManager reflexBoard={reflexBoard} />
      </div>
    </div>
  );
}

export default App;
