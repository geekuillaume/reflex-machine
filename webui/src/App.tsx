import { useEffect, useState } from "react";
import reactLogo from "./assets/react.svg";
import "./App.css";
import { ConnectionManager } from "./ConnectionManager";
import { BUTTONS_COUNT } from "./constants";
import { GameManager } from "./GameManager";
import { ReflexBoard } from "./lib/ReflexBoard";
import { useRerender } from "./lib/useRerender";

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
      <h1>ReflexBoard</h1>
      <div className="card">
        <ConnectionManager reflexBoard={reflexBoard} />
        <GameManager reflexBoard={reflexBoard} />
      </div>
    </div>
  );
}

export default App;
