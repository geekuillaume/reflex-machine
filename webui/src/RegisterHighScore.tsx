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
    const { data } = await supabase
      .from("scores")
      .insert([
        {
          score,
          name,
        },
      ])
      .select("*");
    console.log(data);
    supabase.from("scores");
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
      <p>Score: {(score / 1000).toFixed(2)}</p>
      <p>Rank {rank === -1 ? "..." : rank}</p>
      <p>ENTER YOUR NAME:</p>
      <input
        type="text"
        value={name}
        onChange={(e) => setName(e.target.value)}
      />
      <button onClick={saveScore} disabled={savingState !== "INIT"}>
        SAVE SCORE
      </button>
    </div>
  );
};
