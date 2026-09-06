#pragma once

#include <cstdint>
#include <optional>
#include <vector>

#include "../core/position.h"
#include "../core/move.h"

std::int16_t minimax(Position& position, std::uint8_t depth);

struct SearchStats { std::uint64_t nodes = 0; };
struct SearchResult {
    std::optional<Move> best_move;
    std::int16_t eval;
    SearchStats stats;
};
SearchResult pick_best_move(Position& position, std::uint8_t depth);

struct RankedMove {
    std::uint8_t rank;
    Move move;
    std::int16_t eval;
};
std::vector<RankedMove> rank_moves(Position& position, std::uint8_t depth);
