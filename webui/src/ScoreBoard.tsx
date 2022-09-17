import { useEffect, useState } from "react";
import { supabase, SupabaseScore } from "./lib/supabase";

export const ScoreBoard = () => {
  const [scores, setScores] = useState<SupabaseScore[]>([]);

  let getHash = (str: string) => {
    var hash = 0,
      i,
      chr;
    if (str.length === 0) return hash;
    for (i = 0; i < str.length; i++) {
      chr = str.charCodeAt(i);
      hash = (hash << 5) - hash + chr;
      hash |= 0; // Convert to 32bit integer
    }
    return Math.abs(hash);
  };

  useEffect(() => {
    const updateScores = async () => {
      const { data, error } = await supabase
        .from("scores")
        .select("*")
        .order("score", {
          ascending: true,
        })
        .limit(100);

      if (!error) {
        setScores(data);
      }
    };
    updateScores();

    supabase
      .channel("db-changes")
      .on("postgres_changes", { event: "*", schema: "*" }, () => updateScores())
      .subscribe();
  }, []);

  return (
    <div className="scoreBoard">
      <h2 className="scores--title">Hi-Scores</h2>
      <div className="scoresContainer">
        <div className="scoresBox">
          {scores.map((score) => (
            <p className="scoresLine" key={score.id}>
              <span className={`score${getHash(score.name) % 6} scoresBoxName`}>
                {score.name}
              </span>
              <div className="scoresBoxMid"></div>
              <p className="scoresBoxScore">
                {(score.score / 1000).toFixed(2)}s
              </p>
            </p>
          ))}
        </div>
      </div>
    </div>
  );
};
