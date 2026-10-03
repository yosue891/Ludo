"use client";

import { useEffect, useMemo, useRef, useState } from "react";
import { LudoBoard } from "./LudoBoard";
import { GameSocket } from "@/lib/socket";
import { colorLabel, type RoomState, type ServerMessage } from "@/lib/types";

type Connection = "connecting" | "online" | "offline";

const dicePips: Record<number, number[]> = {
  1: [4],
  2: [0, 8],
  3: [0, 4, 8],
  4: [0, 2, 6, 8],
  5: [0, 2, 4, 6, 8],
  6: [0, 2, 3, 5, 6, 8]
};

function Dice({ value }: { value: number }) {
  const pips = dicePips[value] || [];
  return (
    <div className="dice" aria-label={value ? `Dado: ${value}` : "Dado sin lanzar"}>
      {Array.from({ length: 9 }, (_, index) => <i key={index} className={pips.includes(index) ? "on" : ""} />)}
    </div>
  );
}

function ConnectionBadge({ status }: { status: Connection }) {
  const labels = { connecting: "Conectando", online: "En línea", offline: "Reconectando" };
  return <div className={`connection ${status}`}><span />{labels[status]}</div>;
}

export function LudoApp() {
  const socket = useRef<GameSocket | null>(null);
  const [status, setStatus] = useState<Connection>("connecting");
  const [sessionId, setSessionId] = useState("");
  const [name, setName] = useState("");
  const [joinCode, setJoinCode] = useState("");
  const [room, setRoom] = useState<RoomState | null>(null);
  const [error, setError] = useState("");

  useEffect(() => {
    const client = new GameSocket();
    socket.current = client;
    const storedSession = localStorage.getItem("ludo.session") || crypto.randomUUID();
    const storedName = localStorage.getItem("ludo.name") || `Jugador ${Math.floor(100 + Math.random() * 900)}`;
    localStorage.setItem("ludo.session", storedSession);
    setSessionId(storedSession);
    setName(storedName);

    const onMessage = (message: ServerMessage) => {
      if (message.type === "hello") {
        setSessionId(message.sessionId);
        localStorage.setItem("ludo.session", message.sessionId);
      } else if (message.type === "state") {
        setRoom(message.room);
        setError("");
      } else if (message.type === "error") {
        setError(message.message);
      }
    };

    const unsubscribe = client.subscribe(onMessage);
    const unsubscribeStatus = client.subscribeStatus(setStatus);
    client.connect(storedSession, storedName);
    return () => {
      unsubscribe();
      unsubscribeStatus();
      client.stop();
      socket.current = null;
    };
  }, []);

  useEffect(() => {
    if (!error) return;
    const timer = window.setTimeout(() => setError(""), 3500);
    return () => window.clearTimeout(timer);
  }, [error]);

  const me = useMemo(() => room?.players.find(player => player.id === sessionId), [room, sessionId]);
  const current = room?.players[room.turn];
  const winner = room?.players.find(player => player.id === room.winnerId);
  const isHost = room?.hostId === sessionId;
  const myTurn = current?.id === sessionId;

  const persistName = (value: string) => {
    const clean = value.slice(0, 20);
    setName(clean);
    localStorage.setItem("ludo.name", clean);
  };

  const send = (payload: Record<string, unknown>) => {
    if (!socket.current?.send(payload)) setError("Aún no hay conexión con el servidor.");
  };

  const leave = () => {
    send({ type: "leave" });
    setRoom(null);
  };

  if (!room) {
    return (
      <main className="appRoot">
        <div className="ambient" aria-hidden="true" />
        <header className="topbar">
          <a className="brand" href="#" aria-label="Ludo Live">
            <span className="brandMark"><i /><i /><i /><i /></span>
            <strong>Ludo <em>Live</em></strong>
          </a>
          <ConnectionBadge status={status} />
        </header>

        <section className="hero">
          <div className="heroCopy">
            <div className="eyebrow"><span /> Multijugador en tiempo real</div>
            <h1>El clásico Ludo,<br /><b>hecho para jugar ya.</b></h1>
            <p>Crea una sala, comparte el código y juega con hasta cuatro personas. Sin cuentas, sin esperas.</p>
            <div className="miniPlayers" aria-label="Colores de jugadores">
              <i className="red" /><i className="green" /><i className="yellow" /><i className="blue" />
              <span>2–4 jugadores</span>
            </div>
          </div>

          <div className="lobbyCard">
            <div className="cardHead">
              <span>Tu partida</span>
              <small>RÁPIDO · EN VIVO</small>
            </div>
            <label className="fieldLabel" htmlFor="player-name">Tu nombre</label>
            <input
              id="player-name"
              className="input"
              value={name}
              onChange={event => persistName(event.target.value)}
              placeholder="¿Cómo te llamas?"
              maxLength={20}
              autoComplete="nickname"
            />
            <button className="primaryButton" onClick={() => send({ type: "create_room", name: name.trim() || "Jugador" })} disabled={status !== "online"}>
              <span>Crear partida</span><b>→</b>
            </button>
            <div className="divider"><span>o únete con un código</span></div>
            <div className="joinRow">
              <input
                className="input codeInput"
                value={joinCode}
                onChange={event => setJoinCode(event.target.value.toUpperCase().replace(/[^A-Z0-9]/g, "").slice(0, 6))}
                placeholder="ABC123"
                aria-label="Código de sala"
                maxLength={6}
              />
              <button className="secondaryButton" onClick={() => send({ type: "join_room", code: joinCode, name: name.trim() || "Jugador" })} disabled={joinCode.length !== 6 || status !== "online"}>
                Entrar
              </button>
            </div>
            <p className="privacyNote"><span>◆</span> El estado de la partida vive en el servidor y se sincroniza automáticamente.</p>
          </div>
        </section>
        <footer className="landingFooter"><span>Reconnection ready</span><span>WebSocket realtime</span><span>Server authoritative</span></footer>
        {error && <div className="toast" role="status">{error}</div>}
      </main>
    );
  }

  return (
    <main className="gameRoot">
      <header className="gameTopbar">
        <div className="brand compact">
          <span className="brandMark"><i /><i /><i /><i /></span>
          <strong>Ludo <em>Live</em></strong>
        </div>
        <div className="roomCode">
          <span>SALA</span>
          <button onClick={() => navigator.clipboard?.writeText(room.code)} aria-label={`Copiar código ${room.code}`}>{room.code}<b>⧉</b></button>
        </div>
        <ConnectionBadge status={status} />
      </header>

      <section className="gameLayout">
        <div className="boardArea">
          <div className="turnStrip">
            <div>
              <span className={`turnDot ${current?.color || "red"}`} />
              <div><small>Turno actual</small><strong>{current?.name || "—"}{myTurn ? " · tú" : ""}</strong></div>
            </div>
            <span>{room.phase === "lobby" ? "Esperando jugadores" : room.phase === "move" ? "Elige una ficha" : room.phase === "roll" ? "Lanza el dado" : "Partida terminada"}</span>
          </div>
          <LudoBoard room={room} sessionId={sessionId} onMove={token => send({ type: "move", token })} />
        </div>

        <aside className="controlPanel">
          <div className="panelSection playersSection">
            <div className="sectionTitle"><span>Jugadores</span><small>{room.players.length}/4</small></div>
            <div className="playerList">
              {room.players.map((player, index) => (
                <div className={`playerRow${player.id === sessionId ? " me" : ""}`} key={player.id}>
                  <span className={`avatar ${player.color}`}>{player.name.slice(0, 1).toUpperCase()}</span>
                  <div><strong>{player.name}</strong><small>{player.id === room.hostId ? "Anfitrión" : colorLabel[player.color]}</small></div>
                  <span className={`presence${player.connected ? " online" : ""}`} aria-label={player.connected ? "Conectado" : "Desconectado"} />
                  {room.turn === index && room.phase !== "lobby" && room.phase !== "finished" && <span className="turnTag">TURNO</span>}
                </div>
              ))}
            </div>
          </div>

          {room.phase === "lobby" ? (
            <div className="panelSection actionSection">
              <span className="actionKicker">SALA LISTA</span>
              <h2>{room.players.length < 2 ? "Falta alguien más" : "¿Empezamos?"}</h2>
              <p>Comparte <b>{room.code}</b>. La partida admite hasta cuatro jugadores.</p>
              {isHost ? (
                <button className="primaryButton" disabled={room.players.length < 2} onClick={() => send({ type: "start_game" })}>Iniciar partida <b>→</b></button>
              ) : (
                <div className="waiting"><i /><span>Esperando al anfitrión…</span></div>
              )}
            </div>
          ) : (
            <div className="panelSection diceSection">
              <div className="diceStage">
                <Dice value={room.dice} />
                <div>
                  <small>{myTurn ? "TU TURNO" : "EN JUEGO"}</small>
                  <strong>{room.phase === "finished" ? "Final" : room.dice || "—"}</strong>
                </div>
              </div>

              {room.phase === "finished" ? (
                <div className="winnerBox">
                  <span>★</span>
                  <div><small>GANADOR</small><strong>{winner?.name || "Jugador"}</strong></div>
                </div>
              ) : myTurn && room.phase === "roll" ? (
                <button className="rollButton" onClick={() => send({ type: "roll" })}><span>🎲</span> Lanzar dado</button>
              ) : myTurn && room.phase === "move" ? (
                <div className="chooseHint"><span>↑</span><div><strong>Elige tu ficha</strong><small>Las fichas disponibles están resaltadas.</small></div></div>
              ) : (
                <div className="waiting"><i /><span>Esperando a {current?.name || "otro jugador"}…</span></div>
              )}
            </div>
          )}

          <button className="leaveButton" onClick={leave}>Salir de la sala</button>
        </aside>
      </section>
      {error && <div className="toast" role="status">{error}</div>}
    </main>
  );
}
