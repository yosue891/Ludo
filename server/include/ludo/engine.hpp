#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace ludo {

constexpr int kTokenCount = 4;
constexpr int kTrackLength = 52;
constexpr int kHomeEnd = 57;

enum class Color : std::uint8_t { Red, Green, Yellow, Blue };
enum class Phase : std::uint8_t { Lobby, Roll, Move, Finished };

inline constexpr std::array<std::string_view, 4> kColorNames{"red", "green", "yellow", "blue"};
inline constexpr std::array<int, 4> kStartOffsets{0, 13, 26, 39};
inline constexpr std::array<int, 8> kSafeCells{0, 8, 13, 21, 26, 34, 39, 47};

inline std::string_view color_name(Color color) {
  return kColorNames[static_cast<std::size_t>(color)];
}

inline bool is_safe_global(int cell) {
  return std::find(kSafeCells.begin(), kSafeCells.end(), cell) != kSafeCells.end();
}

inline int global_cell(Color color, int progress) {
  if (progress < 0 || progress >= kTrackLength) return -1;
  return (kStartOffsets[static_cast<std::size_t>(color)] + progress) % kTrackLength;
}

struct Player {
  std::string id;
  std::string name;
  Color color{Color::Red};
  bool connected{true};
  std::array<int, kTokenCount> tokens{-1, -1, -1, -1};
};

struct Result {
  bool ok{false};
  std::string error;
  bool captured{false};
  bool extra_turn{false};

  static Result success(bool captured = false, bool extra_turn = false) {
    return {true, {}, captured, extra_turn};
  }

  static Result fail(std::string message) {
    return {false, std::move(message), false, false};
  }
};

class Game {
 public:
  explicit Game(std::string code) : code_(std::move(code)) {}

  const std::string& code() const { return code_; }
  const std::vector<Player>& players() const { return players_; }
  Phase phase() const { return phase_; }
  std::size_t turn_index() const { return turn_; }
  int dice() const { return dice_; }
  const std::string& host_id() const { return host_id_; }
  const std::string& winner_id() const { return winner_id_; }

  const Player* find_player(std::string_view id) const {
    auto it = std::find_if(players_.begin(), players_.end(), [&](const Player& p) { return p.id == id; });
    return it == players_.end() ? nullptr : &*it;
  }

  Player* find_player(std::string_view id) {
    auto it = std::find_if(players_.begin(), players_.end(), [&](Player& p) { return p.id == id; });
    return it == players_.end() ? nullptr : &*it;
  }

  Result add_player(std::string id, std::string name) {
    if (auto* existing = find_player(id)) {
      existing->connected = true;
      if (!name.empty()) existing->name = std::move(name);
      return Result::success();
    }
    if (phase_ != Phase::Lobby) return Result::fail("La partida ya comenzó.");
    if (players_.size() >= 4) return Result::fail("La sala está llena.");
    if (id.empty()) return Result::fail("Sesión inválida.");
    if (players_.empty()) host_id_ = id;
    players_.push_back({std::move(id), std::move(name), static_cast<Color>(players_.size()), true, {-1, -1, -1, -1}});
    return Result::success();
  }

  void set_connected(std::string_view id, bool value) {
    if (auto* player = find_player(id)) player->connected = value;
  }

  Result leave(std::string_view id) {
    auto it = std::find_if(players_.begin(), players_.end(), [&](const Player& p) { return p.id == id; });
    if (it == players_.end()) return Result::fail("Jugador no encontrado.");
    if (phase_ == Phase::Lobby) {
      players_.erase(it);
      for (std::size_t i = 0; i < players_.size(); ++i) players_[i].color = static_cast<Color>(i);
      host_id_ = players_.empty() ? std::string{} : players_.front().id;
      turn_ = 0;
      return Result::success();
    }
    const bool was_turn = !players_.empty() && players_[turn_].id == id;
    it->connected = false;
    if (was_turn && phase_ != Phase::Finished) {
      dice_ = 0;
      phase_ = Phase::Roll;
      advance_turn();
    }
    return Result::success();
  }

  Result start(std::string_view requester) {
    if (phase_ != Phase::Lobby) return Result::fail("La partida ya comenzó.");
    if (requester != host_id_) return Result::fail("Solo el anfitrión puede iniciar.");
    if (players_.size() < 2) return Result::fail("Se necesitan al menos 2 jugadores.");
    phase_ = Phase::Roll;
    turn_ = 0;
    dice_ = 0;
    return Result::success();
  }

  bool can_move(const Player& player, std::size_t token_index, int die) const {
    if (token_index >= player.tokens.size() || die < 1 || die > 6) return false;
    const int position = player.tokens[token_index];
    if (position == -1) return die == 6;
    if (position >= kHomeEnd) return false;
    return position + die <= kHomeEnd;
  }

  std::vector<int> movable_tokens(const Player& player, int die) const {
    std::vector<int> result;
    for (std::size_t i = 0; i < player.tokens.size(); ++i)
      if (can_move(player, i, die)) result.push_back(static_cast<int>(i));
    return result;
  }

  Result apply_roll(std::string_view player_id, int die) {
    if (phase_ != Phase::Roll) return Result::fail("Ahora debes mover una ficha.");
    if (players_.empty() || players_[turn_].id != player_id) return Result::fail("No es tu turno.");
    if (die < 1 || die > 6) return Result::fail("Dado inválido.");
    dice_ = die;
    if (!movable_tokens(players_[turn_], die).empty()) {
      phase_ = Phase::Move;
      return Result::success();
    }
    if (die != 6) advance_turn();
    dice_ = 0;
    phase_ = Phase::Roll;
    return Result::success(false, die == 6);
  }

  Result move(std::string_view player_id, std::size_t token_index) {
    if (phase_ != Phase::Move) return Result::fail("Primero lanza el dado.");
    if (players_.empty() || players_[turn_].id != player_id) return Result::fail("No es tu turno.");
    Player& player = players_[turn_];
    if (!can_move(player, token_index, dice_)) return Result::fail("Esa ficha no puede moverse.");

    int& position = player.tokens[token_index];
    position = position == -1 ? 0 : position + dice_;

    bool captured = false;
    if (position < kTrackLength) {
      const int target = global_cell(player.color, position);
      if (!is_safe_global(target)) {
        for (auto& opponent : players_) {
          if (opponent.id == player.id) continue;
          for (int& other : opponent.tokens) {
            if (other >= 0 && other < kTrackLength && global_cell(opponent.color, other) == target) {
              other = -1;
              captured = true;
            }
          }
        }
      }
    }

    if (std::all_of(player.tokens.begin(), player.tokens.end(), [](int p) { return p == kHomeEnd; })) {
      winner_id_ = player.id;
      phase_ = Phase::Finished;
      dice_ = 0;
      return Result::success(captured, false);
    }

    const bool extra = dice_ == 6 || captured;
    dice_ = 0;
    if (!extra) advance_turn();
    phase_ = Phase::Roll;
    return Result::success(captured, extra);
  }

 private:
  void advance_turn() {
    if (players_.empty()) return;
    const std::size_t start = turn_;
    do {
      turn_ = (turn_ + 1) % players_.size();
      if (players_[turn_].connected) return;
    } while (turn_ != start);
  }

  std::string code_;
  std::vector<Player> players_;
  Phase phase_{Phase::Lobby};
  std::size_t turn_{0};
  int dice_{0};
  std::string host_id_;
  std::string winner_id_;
};

inline std::string_view phase_name(Phase phase) {
  switch (phase) {
    case Phase::Lobby: return "lobby";
    case Phase::Roll: return "roll";
    case Phase::Move: return "move";
    case Phase::Finished: return "finished";
  }
  return "lobby";
}

}  // namespace ludo
