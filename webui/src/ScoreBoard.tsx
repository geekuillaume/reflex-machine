import { useEffect, useState } from "react";
import { supabase, SupabaseScore } from "./lib/supabase";

export const ScoreBoard = () => {
  const [scores, setScores] = useState<SupabaseScore[]>([]);

  useEffect(() => {
    const updateScores = async () => {
      const { data, error } = await supabase
        .from("scores")
        .select("*")
        .order("score", {
          ascending: true,
        })
        .limit(50);

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
    <div>
      <h2>Scores</h2>
      {scores.map((score) => (
        <div key={score.id}>
          {score.name}: {(score.score / 1000).toFixed(2)}s
        </div>
      ))}
    </div>
  );
};
