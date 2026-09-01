#pragma once

#include <vector>
#include <string>
#include <optional>
#include <chrono>

#include "../core/position.h"
#include "../game/game.h"

std::vector<std::string> construct_pgn_lines(const Game& game);

struct ParsedPly  {
    std::string san_move;
    std::optional<std::chrono::milliseconds> clock_after;
};

struct ParsedPGN {
    std::string white_name;
    std::string black_name;
    std::optional<TimeControl> time_control;
    GameResult result;
    std::optional<Position> starting_position;
    std::vector<ParsedPly> plies;
};

ParsedPGN parse_pgn_document(const std::vector<std::string>& lines);
Game reconstruct_game(const ParsedPGN& pgn_data);
