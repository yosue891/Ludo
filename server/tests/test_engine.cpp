#include <cassert>
#include <iostream>
#include "ludo/engine.hpp"

using ludo::Game;
using ludo::Phase;

static Game two_player_game() {
  Game game("ABC123");
  assert(game.add_player("a", "A").ok);
  assert(game.add_player("b", "B").ok);
  assert(game.start("a").ok);
  return game;
}

static void test_join_and_start() {
  Game game("ROOM01");
  assert(game.add_player("a", "Ada").ok);
  assert(!game.start("a").ok);
  assert(game.add_player("b", "Lin").ok);
  assert(!game.start("b").ok);
  assert(game.start("a").ok);
  assert(game.phase() == Phase::Roll);
}

static void test_requires_six_to_leave_base() {
  auto game = two_player_game();
  assert(game.apply_roll("a", 3).ok);
  assert(game.turn_index() == 1);
  assert(game.apply_roll("b", 6).ok);
  assert(game.move("b", 0).ok);
  assert(game.players()[1].tokens[0] == 0);
  assert(game.turn_index() == 1);
}

static void test_exact_home() {
  auto game = two_player_game();
  auto& p = *game.find_player("a");
  p.tokens[0] = 55;
  assert(game.apply_roll("a", 3).ok);
  assert(game.phase() == Phase::Roll);
  assert(game.turn_index() == 1);
}

static void test_capture_grants_extra_turn() {
  auto game = two_player_game();
  auto& a = *game.find_player("a");
  auto& b = *game.find_player("b");
  a.tokens[0] = 1;
  b.tokens[0] = 41;
  assert(game.apply_roll("a", 1).ok);
  auto result = game.move("a", 0);
  assert(result.ok && result.captured && result.extra_turn);
  assert(b.tokens[0] == -1);
  assert(game.turn_index() == 0);
}

static void test_safe_cell_blocks_capture() {
  auto game = two_player_game();
  auto& a = *game.find_player("a");
  auto& b = *game.find_player("b");
  a.tokens[0] = 7;
  b.tokens[0] = 47;
  assert(game.apply_roll("a", 1).ok);
  auto result = game.move("a", 0);
  assert(result.ok && !result.captured);
  assert(b.tokens[0] == 47);
}

static void test_win() {
  auto game = two_player_game();
  auto& a = *game.find_player("a");
  a.tokens = {57, 57, 57, 56};
  assert(game.apply_roll("a", 1).ok);
  auto result = game.move("a", 3);
  assert(result.ok);
  assert(game.phase() == Phase::Finished);
  assert(game.winner_id() == "a");
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
