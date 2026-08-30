#pragma once

#include <vector>
#include <string>

#include "../game/game.h"

std::vector<std::string> construct_pgn_lines(const Game& game);

struct ParsedPGN {
    std::string white_name;
    std::string black_name;
    GameResult result;
    std::vector<std::string> san_moves;
};

ParsedPGN parse_pgn_document(const std::vector<std::string>& lines);
Game reconstruct_game(const ParsedPGN& pgn_data);
