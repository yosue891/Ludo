# Ludo Live implementation design

## Goal

Create a complete realtime Ludo web app for 2–4 players. The experience must be fast, mobile-friendly and visually polished, with game authority held by the backend.

## Architecture

The repository is split into `frontend/` and `server/`.

The frontend is Next.js + TypeScript. It renders the lobby, room, board and action controls. It persists only a reconnect session id and player display name in local storage. It never generates dice outcomes or accepts moves locally as truth.

The server is C++20 with Crow WebSocket. It owns rooms, player colors, dice rolls, turn order, token positions, captures, safe cells and victory. Every accepted mutation broadcasts a complete room snapshot. This keeps the client simple and makes reconnect deterministic.

## Rules

- 2–4 players.
- A six is required to leave base.
- Exact roll is required to reach the final home position.
- Rolling six grants another turn.
- Capturing grants another turn.
- Safe cells cannot be captured.
- A winner is declared when all four tokens reach home.
- Three-sixes and blockades are intentionally omitted from v1 to keep the rules predictable.

## Realtime protocol

Clients identify with `hello`, then create or join a room. Mutations use `start_game`, `roll`, `move`, and `leave`. The server sends `hello`, `state`, `error`, and `pong`.

## Reliability

The reconnect id is stable in browser storage. Disconnect marks a player offline without deleting them. Reconnecting with the same session restores the room. Invalid messages and out-of-turn actions return structured errors instead of mutating state.

## Testing

The rules engine is dependency-free and covered by a standalone C++ test executable. CI builds the server, runs its tests and builds/typechecks the frontend.
