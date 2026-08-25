#pragma once

#include <vector>
#include <string>

#include "../game/game.h"

std::vector<std::string> construct_pgn_lines(const GameState& game);
