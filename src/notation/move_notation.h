#pragma once

#include <string>
#include <string_view>

#include "../core/move.h"
#include "../core/position.h"

enum class MoveNotation { Uci, San };

std::string move_notation(Move move, const Position& position, MoveNotation notation);
Move resolve_move(std::string_view move, const Position& position, MoveNotation notation);
