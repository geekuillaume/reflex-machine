import { GameState } from "./types";

export const delay = (val: number) => {
  return new Promise((resolve) => setTimeout(resolve, val));
};

export const convertBoolsToUint16 = (bools: boolean[]) => {
  const bitarray = new Uint16Array(1);
  bools.forEach((val, i) => {
    if (val) {
      bitarray[0] |= 1 << i;
    }
  });

  return bitarray;
};

export const computeGameScore = (gameState: GameState) =>
  gameState.delaySum + gameState.missed * 1500;
