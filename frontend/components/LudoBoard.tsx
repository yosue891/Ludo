"use client";

import type { CSSProperties } from "react";
import type { Color, Player, RoomState } from "@/lib/types";

type Coord = readonly [number, number];

const TRACK: Coord[] = [
  [6,1],[6,2],[6,3],[6,4],[6,5],[5,6],[4,6],[3,6],[2,6],[1,6],[0,6],[0,7],[0,8],
  [1,8],[2,8],[3,8],[4,8],[5,8],[6,9],[6,10],[6,11],[6,12],[6,13],[6,14],[7,14],[8,14],
  [8,13],[8,12],[8,11],[8,10],[8,9],[9,8],[10,8],[11,8],[12,8],[13,8],[14,8],[14,7],[14,6],
  [13,6],[12,6],[11,6],[10,6],[9,6],[8,5],[8,4],[8,3],[8,2],[8,1],[8,0],[7,0],[6,0]
];

const START: Record<Color, number> = { red: 0, green: 13, yellow: 26, blue: 39 };
const SAFE = new Set([0, 8, 13, 21, 26, 34, 39, 47]);
const LANES: Record<Color, Coord[]> = {
  red: [[7,1],[7,2],[7,3],[7,4],[7,5],[7,6]],
  green: [[1,7],[2,7],[3,7],[4,7],[5,7],[6,7]],
  yellow: [[7,13],[7,12],[7,11],[7,10],[7,9],[7,8]],
  blue: [[13,7],[12,7],[11,7],[10,7],[9,7],[8,7]]
};
const BASES: Record<Color, Coord[]> = {
  red: [[1.65,1.65],[1.65,4.35],[4.35,1.65],[4.35,4.35]],
  green: [[1.65,10.65],[1.65,13.35],[4.35,10.65],[4.35,13.35]],
  yellow: [[10.65,10.65],[10.65,13.35],[13.35,10.65],[13.35,13.35]],
  blue: [[10.65,1.65],[10.65,4.35],[13.35,1.65],[13.35,4.35]]
};

function tokenCoord(player: Player, token: number): Coord {
  const position = player.tokens[token];
  if (position < 0) return BASES[player.color][token];
  if (position < 52) return TRACK[(START[player.color] + position) % 52];
  return LANES[player.color][Math.min(position - 52, 5)];
}

function canMove(position: number, dice: number) {
  if (!dice) return false;
  if (position < 0) return dice === 6;
  return position < 57 && position + dice <= 57;
}

function cellClass(row: number, col: number) {
  const trackIndex = TRACK.findIndex(([r, c]) => r === row && c === col);
  if (trackIndex >= 0) return `track${SAFE.has(trackIndex) ? " safe" : ""}`;
  for (const color of Object.keys(LANES) as Color[])
    if (LANES[color].some(([r, c]) => r === row && c === col)) return `lane ${color}`;
  if (row >= 6 && row <= 8 && col >= 6 && col <= 8) return "center";
  if (row <= 5 && col <= 5) return "yard red";
  if (row <= 5 && col >= 9) return "yard green";
  if (row >= 9 && col >= 9) return "yard yellow";
  if (row >= 9 && col <= 5) return "yard blue";
  return "void";
}

export function LudoBoard({
  room,
  sessionId,
  onMove
}: {
  room: RoomState;
  sessionId: string;
  onMove: (token: number) => void;
}) {
  const me = room.players.find(player => player.id === sessionId);
  const myTurn = !!me && room.players[room.turn]?.id === sessionId;
  const canChoose = myTurn && room.phase === "move";

  return (
    <div className="boardShell">
      <div className="board" role="group" aria-label="Tablero de Ludo">
        {Array.from({ length: 225 }, (_, index) => {
          const row = Math.floor(index / 15);
          const col = index % 15;
          const klass = cellClass(row, col);
          const trackIndex = TRACK.findIndex(([r, c]) => r === row && c === col);
          return (
            <div className={`boardCell ${klass}`} key={index}>
              {trackIndex >= 0 && SAFE.has(trackIndex) && <span className="safeMark">✦</span>}
            </div>
          );
        })}
        <div className="yardPlate red" aria-hidden="true"><i /><i /><i /><i /></div>
        <div className="yardPlate green" aria-hidden="true"><i /><i /><i /><i /></div>
        <div className="yardPlate yellow" aria-hidden="true"><i /><i /><i /><i /></div>
        <div className="yardPlate blue" aria-hidden="true"><i /><i /><i /><i /></div>
        <div className="centerMark" aria-hidden="true"><span>★</span></div>
        {room.players.flatMap(player =>
          player.tokens.map((_, token) => {
            const [row, col] = tokenCoord(player, token);
            const movable = player.id === sessionId && canChoose && canMove(player.tokens[token], room.dice);
            const jitter = token % 4;
            const style = {
              "--token-x": `${((col + .5) / 15) * 100}%`,
              "--token-y": `${((row + .5) / 15) * 100}%`,
              "--stack-x": `${[-5, 5, -5, 5][jitter]}px`,
              "--stack-y": `${[-5, -5, 5, 5][jitter]}px`
            } as CSSProperties;
            return (
              <button
                className={`token ${player.color}${movable ? " movable" : ""}`}
                style={style}
                key={`${player.id}-${token}`}
                onClick={() => movable && onMove(token)}
                disabled={!movable}
                aria-label={`${player.name}, ficha ${token + 1}${movable ? ", mover" : ""}`}
              >
                <span />
              </button>
            );
          })
        )}
      </div>
    </div>
  );
}
