import type { ServerMessage } from "./types";

type Listener = (message: ServerMessage) => void;
type StatusListener = (status: "connecting" | "online" | "offline") => void;

export class GameSocket {
  private ws: WebSocket | null = null;
  private reconnectTimer: number | null = null;
  private stopped = false;
  private listeners = new Set<Listener>();
  private statusListeners = new Set<StatusListener>();
  private sessionId = "";
  private name = "";

  connect(sessionId: string, name: string) {
    this.sessionId = sessionId;
    this.name = name;
    this.stopped = false;
    this.open();
  }

  stop() {
    this.stopped = true;
    if (this.reconnectTimer) window.clearTimeout(this.reconnectTimer);
    this.ws?.close();
    this.ws = null;
  }

  subscribe(listener: Listener) {
    this.listeners.add(listener);
    return () => this.listeners.delete(listener);
  }

  subscribeStatus(listener: StatusListener) {
    this.statusListeners.add(listener);
    return () => this.statusListeners.delete(listener);
  }

  send(payload: Record<string, unknown>) {
    if (this.ws?.readyState !== WebSocket.OPEN) return false;
    this.ws.send(JSON.stringify(payload));
    return true;
  }

  private open() {
    if (this.stopped) return;
    this.emitStatus("connecting");
    const url = process.env.NEXT_PUBLIC_WS_URL || "ws://localhost:8080/ws";
    const ws = new WebSocket(url);
    this.ws = ws;

    ws.onopen = () => {
      this.emitStatus("online");
      ws.send(JSON.stringify({ type: "hello", sessionId: this.sessionId, name: this.name }));
    };

    ws.onmessage = event => {
      try {
        const message = JSON.parse(String(event.data)) as ServerMessage;
        this.listeners.forEach(listener => listener(message));
      } catch {
        this.listeners.forEach(listener => listener({ type: "error", message: "Respuesta inválida del servidor." }));
      }
    };

    ws.onclose = () => {
      if (this.ws === ws) this.ws = null;
      this.emitStatus("offline");
      if (!this.stopped) this.reconnectTimer = window.setTimeout(() => this.open(), 1200);
    };

    ws.onerror = () => ws.close();
  }

  private emitStatus(status: "connecting" | "online" | "offline") {
    this.statusListeners.forEach(listener => listener(status));
  }
}
