#include <cstdlib>
#include <iostream>
#include <string_view>
#include "ludo/engine.hpp"

using ludo::Game;
using ludo::Phase;

static void check(bool condition, std::string_view message) {
  if (condition) return;
  std::cerr << "FAIL: " << message << '\n';
  std::exit(1);
}

static Game two_player_game() {
  Game game("ABC123");
  check(game.add_player("a", "A").ok, "add player a");
  check(game.add_player("b", "B").ok, "add player b");
  check(game.start("a").ok, "host starts game");
  return game;
}

static void test_join_and_start() {
  Game game("ROOM01");
  check(game.add_player("a", "Ada").ok, "host joins");
  check(!game.start("a").ok, "cannot start alone");
  check(game.add_player("b", "Lin").ok, "second player joins");
  check(!game.start("b").ok, "non-host cannot start");
  check(game.start("a").ok, "host can start");
  check(game.phase() == Phase::Roll, "game enters roll phase");
}

static void test_requires_six_to_leave_base() {
  auto game = two_player_game();
  check(game.apply_roll("a", 3).ok, "a rolls three");
  check(game.turn_index() == 1, "turn advances when no move exists");
  check(game.apply_roll("b", 6).ok, "b rolls six");
  check(game.move("b", 0).ok, "b leaves base");
  check(game.players()[1].tokens[0] == 0, "token is on starting square");
  check(game.turn_index() == 1, "six grants extra turn");
}

static void test_exact_home() {
  auto game = two_player_game();
  auto* player = game.find_player("a");
  check(player != nullptr, "player a exists");
  player->tokens[0] = 55;
  check(game.apply_roll("a", 3).ok, "oversized home roll is handled");
  check(game.phase() == Phase::Roll, "no illegal move phase is offered");
  check(game.turn_index() == 1, "turn advances after impossible exact-home roll");
}

static void test_capture_grants_extra_turn() {
  auto game = two_player_game();
  auto* a = game.find_player("a");
  auto* b = game.find_player("b");
  check(a && b, "capture players exist");
  a->tokens[0] = 1;
  b->tokens[0] = 41;
  check(game.apply_roll("a", 1).ok, "capture roll accepted");
  auto result = game.move("a", 0);
  check(result.ok && result.captured && result.extra_turn, "capture grants extra turn");
  check(b->tokens[0] == -1, "captured token returns to base");
  check(game.turn_index() == 0, "capturing player keeps turn");
}

static void test_safe_cell_blocks_capture() {
  auto game = two_player_game();
  auto* a = game.find_player("a");
  auto* b = game.find_player("b");
  check(a && b, "safe-cell players exist");
  a->tokens[0] = 7;
  b->tokens[0] = 47;
  check(game.apply_roll("a", 1).ok, "safe-cell roll accepted");
  auto result = game.move("a", 0);
  check(result.ok && !result.captured, "safe cell prevents capture");
  check(b->tokens[0] == 47, "opponent remains on safe cell");
}

static void test_win() {
  auto game = two_player_game();
  auto* a = game.find_player("a");
  check(a != nullptr, "winning player exists");
  a->tokens = {57, 57, 57, 56};
  check(game.apply_roll("a", 1).ok, "winning roll accepted");
  auto result = game.move("a", 3);
  check(result.ok, "winning move accepted");
  check(game.phase() == Phase::Finished, "game finishes");
  check(game.winner_id() == "a", "winner is recorded");
}

int main() {
  test_join_and_start();
  test_requires_six_to_leave_base();
  test_exact_home();
  test_capture_grants_extra_turn();
  test_safe_cell_blocks_capture();
  test_win();
  std::cout << "ludo_engine_tests: ok\n";
}
