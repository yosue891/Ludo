#define CROW_ENFORCE_WS_SPEC
#include <crow.h>

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <mutex>
#include <random>
#include <string>
#include <string_view>
#include <unordered_map>

#include "ludo/engine.hpp"

namespace {

using Connection = crow::websocket::connection;

std::string random_text(std::size_t length, std::string_view alphabet) {
  static thread_local std::mt19937_64 rng(std::random_device{}());
  std::uniform_int_distribution<std::size_t> pick(0, alphabet.size() - 1);
  std::string out;
  out.reserve(length);
  while (out.size() < length) out.push_back(alphabet[pick(rng)]);
  return out;
}

std::string session_id() {
  return random_text(24, "abcdefghijklmnopqrstuvwxyz0123456789");
}

std::string room_code() {
  return random_text(6, "ABCDEFGHJKLMNPQRSTUVWXYZ23456789");
}

std::string clean_name(std::string value) {
  value.erase(std::remove_if(value.begin(), value.end(), [](unsigned char c) { return c < 32; }), value.end());
  if (value.size() > 40) value.resize(40);
  return value.empty() ? "Jugador" : value;
}

std::string upper_code(std::string value) {
  std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
  return value;
}

int die_roll() {
  static thread_local std::mt19937 rng(std::random_device{}());
  return std::uniform_int_distribution<int>(1, 6)(rng);
}

crow::json::wvalue state_json(const ludo::Game& game) {
  crow::json::wvalue root;
  root["type"] = "state";
  auto& room = root["room"];
  room["code"] = game.code();
  room["hostId"] = game.host_id();
  room["phase"] = std::string(ludo::phase_name(game.phase()));
  room["turn"] = static_cast<unsigned>(game.turn_index());
  room["dice"] = game.dice();
  room["winnerId"] = game.winner_id();

  const auto& players = game.players();
  for (unsigned i = 0; i < players.size(); ++i) {
    const auto& player = players[i];
    auto& out = room["players"][i];
    out["id"] = player.id;
    out["name"] = player.name;
    out["color"] = std::string(ludo::color_name(player.color));
    out["connected"] = player.connected;
    for (unsigned token = 0; token < player.tokens.size(); ++token) out["tokens"][token] = player.tokens[token];
  }
  if (players.empty()) room["players"] = crow::json::wvalue::list{};
  return root;
}

class Server {
 public:
  void open(Connection&) {}

  void close(Connection& connection) {
    std::scoped_lock lock(mutex_);
    auto it = session_by_connection_.find(&connection);
    if (it == session_by_connection_.end()) return;
    const std::string session = it->second;
    if (auto live = connection_by_session_.find(session); live != connection_by_session_.end() && live->second == &connection)
      connection_by_session_.erase(live);
    session_by_connection_.erase(it);
    if (auto room_it = room_by_session_.find(session); room_it != room_by_session_.end()) {
      auto game_it = rooms_.find(room_it->second);
      if (game_it != rooms_.end()) {
        game_it->second.set_connected(session, false);
        broadcast_locked(game_it->first);
      }
    }
  }

  void message(Connection& connection, const std::string& data, bool binary) {
    if (binary) return send_error(connection, "Solo se aceptan mensajes JSON.");
    auto body = crow::json::load(data);
    if (!body || !body.has("type") || body["type"].t() != crow::json::type::String)
      return send_error(connection, "Mensaje inválido.");

    std::scoped_lock lock(mutex_);
    const std::string type = body["type"].s();

    if (type == "hello") return hello_locked(connection, body);

    const auto session_it = session_by_connection_.find(&connection);
    if (session_it == session_by_connection_.end()) return send_error_locked(connection, "Primero identifica tu sesión.");
    const std::string& session = session_it->second;

    if (type == "ping") {
      crow::json::wvalue out;
      out["type"] = "pong";
      connection.send_text(out.dump());
      return;
    }

    if (type == "create_room") return create_room_locked(connection, session, body);
    if (type == "join_room") return join_room_locked(connection, session, body);
    if (type == "leave") return leave_locked(connection, session);

    auto room_ref = room_by_session_.find(session);
    if (room_ref == room_by_session_.end()) return send_error_locked(connection, "No estás en una sala.");
    auto game_it = rooms_.find(room_ref->second);
    if (game_it == rooms_.end()) return send_error_locked(connection, "La sala ya no existe.");
    auto& game = game_it->second;

    ludo::Result result;
    if (type == "start_game") {
      result = game.start(session);
    } else if (type == "roll") {
      result = game.apply_roll(session, die_roll());
    } else if (type == "move") {
      if (!body.has("token") || body["token"].t() != crow::json::type::Number)
        return send_error_locked(connection, "Ficha inválida.");
      const int token = body["token"].i();
      if (token < 0 || token >= ludo::kTokenCount) return send_error_locked(connection, "Ficha inválida.");
      result = game.move(session, static_cast<std::size_t>(token));
    } else {
      return send_error_locked(connection, "Acción desconocida.");
    }

    if (!result.ok) return send_error_locked(connection, result.error);
    broadcast_locked(game.code());
  }

 private:
  void hello_locked(Connection& connection, const crow::json::rvalue& body) {
    std::string session;
    if (body.has("sessionId") && body["sessionId"].t() == crow::json::type::String) session = body["sessionId"].s();
    if (session.size() < 8 || session.size() > 64) session = session_id();

    if (auto old = connection_by_session_.find(session); old != connection_by_session_.end() && old->second != &connection)
      session_by_connection_.erase(old->second);

    session_by_connection_[&connection] = session;
    connection_by_session_[session] = &connection;

    crow::json::wvalue response;
    response["type"] = "hello";
    response["sessionId"] = session;
    connection.send_text(response.dump());

    if (auto room_ref = room_by_session_.find(session); room_ref != room_by_session_.end()) {
      if (auto game_it = rooms_.find(room_ref->second); game_it != rooms_.end()) {
        game_it->second.set_connected(session, true);
        if (body.has("name") && body["name"].t() == crow::json::type::String) {
          if (auto* player = game_it->second.find_player(session)) player->name = clean_name(body["name"].s());
        }
        broadcast_locked(game_it->first);
      }
    }
  }

  void create_room_locked(Connection& connection, const std::string& session, const crow::json::rvalue& body) {
    if (auto existing = room_by_session_.find(session); existing != room_by_session_.end()) {
      if (auto game = rooms_.find(existing->second); game != rooms_.end()) {
        connection.send_text(state_json(game->second).dump());
        return;
      }
      room_by_session_.erase(existing);
    }

    std::string code;
    do code = room_code(); while (rooms_.contains(code));
    ludo::Game game(code);
    const std::string name = body.has("name") && body["name"].t() == crow::json::type::String ? clean_name(body["name"].s()) : "Jugador";
    auto result = game.add_player(session, name);
    if (!result.ok) return send_error_locked(connection, result.error);
    rooms_.emplace(code, std::move(game));
    room_by_session_[session] = code;
    broadcast_locked(code);
  }

  void join_room_locked(Connection& connection, const std::string& session, const crow::json::rvalue& body) {
    if (!body.has("code") || body["code"].t() != crow::json::type::String)
      return send_error_locked(connection, "Escribe un código de sala.");
    if (room_by_session_.contains(session)) return send_error_locked(connection, "Ya estás en una sala.");

    const std::string code = upper_code(body["code"].s());
    auto game_it = rooms_.find(code);
    if (game_it == rooms_.end()) return send_error_locked(connection, "No encontramos esa sala.");
    const std::string name = body.has("name") && body["name"].t() == crow::json::type::String ? clean_name(body["name"].s()) : "Jugador";
    auto result = game_it->second.add_player(session, name);
    if (!result.ok) return send_error_locked(connection, result.error);
    room_by_session_[session] = code;
    broadcast_locked(code);
  }

  void leave_locked(Connection& connection, const std::string& session) {
    auto room_ref = room_by_session_.find(session);
    if (room_ref == room_by_session_.end()) return;
    const std::string code = room_ref->second;
    auto game_it = rooms_.find(code);
    room_by_session_.erase(room_ref);
    if (game_it == rooms_.end()) return;

    const bool lobby = game_it->second.phase() == ludo::Phase::Lobby;
    game_it->second.leave(session);
    if (lobby && game_it->second.players().empty()) {
      rooms_.erase(game_it);
      return;
    }
    broadcast_locked(code);
  }

  void broadcast_locked(const std::string& code) {
    auto game_it = rooms_.find(code);
    if (game_it == rooms_.end()) return;
    const std::string payload = state_json(game_it->second).dump();
    for (const auto& player : game_it->second.players()) {
      auto membership = room_by_session_.find(player.id);
      if (membership == room_by_session_.end() || membership->second != code) continue;
      auto connection = connection_by_session_.find(player.id);
      if (connection != connection_by_session_.end()) connection->second->send_text(payload);
    }
  }

  void send_error(Connection& connection, std::string_view message) {
    std::scoped_lock lock(mutex_);
    send_error_locked(connection, message);
  }

  static void send_error_locked(Connection& connection, std::string_view message) {
    crow::json::wvalue out;
    out["type"] = "error";
    out["message"] = std::string(message);
    connection.send_text(out.dump());
  }

  std::mutex mutex_;
  std::unordered_map<std::string, ludo::Game> rooms_;
  std::unordered_map<std::string, std::string> room_by_session_;
  std::unordered_map<Connection*, std::string> session_by_connection_;
  std::unordered_map<std::string, Connection*> connection_by_session_;
};

}  // namespace

int main() {
  crow::SimpleApp app;
  Server server;

  CROW_ROUTE(app, "/health")([] {
    crow::json::wvalue out;
    out["ok"] = true;
    out["service"] = "ludo-live";
    return out;
  });

  CROW_WEBSOCKET_ROUTE(app, "/ws")
      .onopen([&server](Connection& connection) { server.open(connection); })
      .onclose([&server](Connection& connection, const std::string&, std::uint16_t) { server.close(connection); })
      .onmessage([&server](Connection& connection, const std::string& data, bool binary) { server.message(connection, data, binary); });

  const char* port_env = std::getenv("PORT");
  const std::uint16_t port = port_env ? static_cast<std::uint16_t>(std::stoi(port_env)) : 8080;
  app.port(port).multithreaded().run();
}
