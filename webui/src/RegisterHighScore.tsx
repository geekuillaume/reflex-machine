import { useEffect, useState } from "react";
import { computeGameScore } from "./lib/lib";
import { supabase } from "./lib/supabase";
import { GameState } from "./lib/types";

export const RegisterHighScore = ({ gameState }: { gameState: GameState }) => {
  const [rank, setRank] = useState(-1);
  const [name, setName] = useState("");
  const [savingState, setSavingState] = useState<"INIT" | "LOADING" | "DONE">(
    "INIT"
  );
  const score = Math.floor(computeGameScore(gameState));

  useEffect(() => {
    supabase
      .from("scores")
      .select("id", { count: "exact" })
      .filter("score", "lt", score)
      .then((res) => {
        setRank(res.count! + 1);
      });
  }, [score]);

  const saveScore = async () => {
    setSavingState("LOADING");
    await supabase.from("scores").insert([
      {
        score,
        name,
      },
    ]);
    setSavingState("DONE");
  };

  if (savingState === "DONE") {
    return (
      <div>
        <p>You just got the {rank} place!</p>
        <p>Your score was saved!</p>
      </div>
    );
  }

  return (
    <div>
      {rank == 1 && (
        <p className="newHighScore">
          New High Score : {(score / 1000).toFixed(2)}s !
        </p>
      )}
      <table className="tableHighScore">
        <tbody>
          {rank !== 1 && (
            <tr>
              <td className="tableHSLeft">
                <span className="smallLabel">Your Score </span>{" "}
              </td>
              <td className="tableHSRight">{(score / 1000).toFixed(2)}s</td>
            </tr>
          )}
          {rank !== 1 && (
            <tr>
              <td className="tableHSLeft">
                <span className="smallLabel">Your Rank </span>{" "}
              </td>
              <td className="tableHSRight">{rank === -1 ? "..." : rank}</td>
            </tr>
          )}
        </tbody>
      </table>
      <br />
      <p className="smallLabel">Enter your name</p>
      <input
        className="nameInput"
        type="text"
        value={name.toUpperCase()}
        onChange={(e) => setName(e.target.value.slice(0, 3).toUpperCase())}
        maxLength={3}
        autoFocus
      />
      <br />
      <button
        className="registerScoreButton"
        onClick={saveScore}
        disabled={savingState !== "INIT"}
      >
        SAVE SCORE
      </button>
    </div>
  );
};
