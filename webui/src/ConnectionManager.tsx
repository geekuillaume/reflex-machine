import { useEffect, useRef, useState } from "react";
import { BUTTONS_COUNT } from "./constants";
import { convertBoolsToUint16, delay } from "./lib";
import { ReflexBoard } from "./lib/ReflexBoard";
import { ReceivedMessage } from "./types";

const IP = "192.168.1.122";

export const ConnectionManager = ({
  reflexBoard,
}: {
  reflexBoard: ReflexBoard;
}) => {
  const handleConnectClick = () => {
    reflexBoard.connect(IP);
  };
  const handleDisconnedClick = () => {
    reflexBoard.disconnect();
  };

  return (
    <div>
      ConnectionManager
      <button onClick={handleConnectClick} disabled={!!reflexBoard.websocket}>
        Connect
      </button>
      <button onClick={handleDisconnedClick} disabled={!reflexBoard.websocket}>
        Disconnected
      </button>
      <p>{!!reflexBoard.websocket ? "Connected" : "Disconnected"}</p>
    </div>
  );
};
