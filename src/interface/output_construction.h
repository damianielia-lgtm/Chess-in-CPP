#pragma once

#include <string>
#include <chrono>
#include <vector>
#include <optional>

#include "../core/position.h"
#include "../application/game.h"

std::string format_time(std::chrono::milliseconds duration);

std::vector<std::string> construct_board_lines(const Position& position, bool flip);

std::vector<std::string> construct_game_lines(const GameState& game, bool show_clock, std::optional<std::string> error);
