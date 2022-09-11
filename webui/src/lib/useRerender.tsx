import { useCallback, useState } from "react";

export const useRerender = () => {
  const [, setDummy] = useState<any>();
  const forceUpdate = useCallback(() => {
    const newRef = {};
    setDummy(newRef);
  }, []);
  return forceUpdate;
};
