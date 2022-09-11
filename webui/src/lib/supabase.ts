import { createClient } from "@supabase/supabase-js";

export const supabase = createClient(
  "https://rhuasqrwmadlufqznqqj.supabase.co",
  "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6InJodWFzcXJ3bWFkbHVmcXpucXFqIiwicm9sZSI6ImFub24iLCJpYXQiOjE2NjI5MTMwODYsImV4cCI6MTk3ODQ4OTA4Nn0.7v3UKA7gcFZ6CzZYXylTz3DDN4HE_h4U9YrbMhfvAsQ"
);

export interface SupabaseScore {
  id: string;
  score: number;
  name: string;
  createdAt: Date;
}
