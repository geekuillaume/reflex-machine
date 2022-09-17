import { FormEvent, useEffect, useState } from "react";
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

  useEffect(() => {
    supabase
      .from("scores")
      .select("id", { count: "exact" })
      .filter("score", "lt", score)
      .then((res) => {
        setRank(res.count! + 1);
      });
  }, [score]);

  const saveScore = async (e: FormEvent<HTMLFormElement>) => {
    e.preventDefault();
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
    const ordinals = ["", "st", "nd", "rd"];

    return (
      <div>
        <p>
          You just got the {rank}
          {rank < ordinals.length ? ordinals[rank] : "th"} place!
        </p>
        <p>Your score was saved!</p>
      </div>
    );
  }

  return (
    <div>
      {rank == 1 && (
        <p className="newHighScore">
          New High Score: {(score / 1000).toFixed(2)}s !
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
      <p className="smallLabel">
        You made <span style={{ color: missColor }}>{gameState.missed}</span>{" "}
        mistake{gameState.missed > 1 ? "s" : ""}
      </p>
      <br />
      <p className="smallLabel">Enter your name</p>
      <form onSubmit={saveScore}>
        <input
          className="nameInput"
          type="text"
          value={name.toUpperCase()}
          onChange={(e) => setName(e.target.value.slice(0, 3).toUpperCase())}
          maxLength={3}
          autoFocus
          onKeyDown={(e) => console.log(e)}
        />
        <br />
        <button
          className="registerScoreButton"
          type="submit"
          disabled={savingState !== "INIT"}
        >
          SAVE SCORE
        </button>
      </form>
    </div>
  );
};
