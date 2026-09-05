#pragma once

#include <cstdint>
#include <optional>
#include <vector>

#include "../core/position.h"
#include "../core/move.h"

std::int16_t minimax(Position& position, std::uint8_t depth);

std::optional<Move> pick_best_move(Position& position, std::uint8_t depth);

struct RankedMove { Move move; std::int16_t eval; };
std::vector<RankedMove> rank_moves(Position& position, std::uint8_t depth);
