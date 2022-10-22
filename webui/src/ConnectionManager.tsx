import { useEffect, useRef, useState } from "react";
import { BUTTONS_COUNT } from "./constants";
import { convertBoolsToUint16, delay } from "./lib/lib";
import { ReflexBoard } from "./lib/ReflexBoard";
import { ReceivedMessage } from "./lib/types";

const IP = "192.168.1.122";
const CONNECT_BY_DEFAULT = true;

export const ConnectionManager = ({
  reflexBoard,
}: {
  reflexBoard: ReflexBoard;
}) => {
  const handleConnectClick = () => {
    if (reflexBoard.websocket) {
      reflexBoard.disconnect();
    } else {
      reflexBoard.connect(IP);
    }
  };

  useEffect(() => {
    if (CONNECT_BY_DEFAULT) {
      reflexBoard.connect(IP);
    }
  }, [reflexBoard]);

  let buttonBorder = !reflexBoard.websocket ? "#35d0ba" : "#d92027";
  return (
    <div className="connexionManager">
      <button
        onClick={handleConnectClick}
        className="connectButton"
        style={{ border: buttonBorder + " solid 2px" }}
      >
        {!reflexBoard.websocket ? "Connect" : "Disconnect"}
      </button>
    </div>
  );
};
