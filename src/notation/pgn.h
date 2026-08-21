#pragma once

#include <vector>
#include <string>

#include "../application/game.h"

std::vector<std::string> construct_pgn_lines(const GameState& game);
