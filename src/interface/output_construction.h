#pragma once

#include <string>
#include <chrono>
#include <vector>
#include <optional>

#include "../core/position.h"
#include "../game/game.h"

std::string format_time(std::chrono::milliseconds duration);

std::vector<std::string> construct_board_lines(
    const Position& position,
    bool flip,
    std::optional<Move> move = std::nullopt
);

std::vector<std::string> construct_game_lines(
    const GameSnapshot& snapshot,
    std::optional<std::string> error,
    std::optional<GameResult> result_message,
    const GameMetadata& metadata,
    bool flip_board
);
