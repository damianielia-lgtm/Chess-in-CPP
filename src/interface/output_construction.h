#pragma once

#include <string>
#include <chrono>
#include <vector>
#include <optional>

#include "../core/position.h"
#include "../game/game.h"
#include "../config.h"

std::string format_time(std::chrono::milliseconds duration);

std::vector<std::string> construct_board_lines(
    const Position& position,
    BoardOrientation board_orientation,
    std::optional<Move> move = std::nullopt
);

std::vector<std::string> construct_game_lines(
    const GameSnapshot& snapshot,
    std::optional<std::string> error,
    std::optional<GameResult> result_message,
    const GameMetadata& metadata,
    BoardOrientation board_orientation
);

std::vector<std::string> construct_config_show_lines(const ConfigData& config);
