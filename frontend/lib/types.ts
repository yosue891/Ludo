export type Color = "red" | "green" | "yellow" | "blue";
export type Phase = "lobby" | "roll" | "move" | "finished";

export type Player = {
  id: string;
  name: string;
  color: Color;
  connected: boolean;
  tokens: number[];
};

export type RoomState = {
  code: string;
  hostId: string;
  phase: Phase;
  turn: number;
  dice: number;
  winnerId: string;
  players: Player[];
};

export type ServerMessage =
  | { type: "hello"; sessionId: string }
  | { type: "state"; room: RoomState }
  | { type: "error"; message: string }
  | { type: "pong" };

export const colorLabel: Record<Color, string> = {
  red: "Rojo",
  green: "Verde",
  yellow: "Amarillo",
  blue: "Azul"
};
