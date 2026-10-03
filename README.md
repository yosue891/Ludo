# Ludo Live

A realtime multiplayer Ludo game built as a responsive web app.

## Stack

- Frontend: Next.js 16.3.8, React 19, TypeScript
- Backend: C++20, Crow 1.3.4, WebSocket
- Transport: JSON messages over WebSocket
- Game model: authoritative server state

## Features

- 2–4 player rooms with six-character codes
- Server-side dice, turn validation, captures, safe cells and home lanes
- Extra turn on six or capture
- Session-based reconnect
- Responsive 15×15 Ludo board
- Mobile-first controls and connection states
- Health endpoint at `/health`

## Run locally

### Backend

```bash
cmake -S server -B server/build -DCMAKE_BUILD_TYPE=Release
cmake --build server/build -j
./server/build/ludo_server
```

The server listens on `0.0.0.0:8080` by default. Set `PORT` to override it.

### Frontend

```bash
cd frontend
npm install
NEXT_PUBLIC_WS_URL=ws://localhost:8080/ws npm run dev
```

Open `http://localhost:3000`.

## Protocol

Client messages:

- `hello` — `{ type, sessionId?, name? }`
- `create_room` — `{ type, name }`
- `join_room` — `{ type, code, name }`
- `start_game` — `{ type }`
- `roll` — `{ type }`
- `move` — `{ type, token }`
- `leave` — `{ type }`
- `ping` — `{ type }`

Server messages:

- `hello` — session identity
- `state` — full authoritative room snapshot
- `error` — action rejected with a safe message
- `pong` — keepalive acknowledgement
