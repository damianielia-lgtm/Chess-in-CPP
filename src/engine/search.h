#pragma once

#include <cstdint>
#include <optional>
#include <vector>
#include <array>
#include <cassert>
#include <chrono>

#include "../core/position.h"
#include "../core/move.h"
#include "../movegen/legal_moves.h"

struct SearchedMove {
    std::optional<Move> move;
    std::int16_t eval;
};

struct NullObserver {
    static constexpr bool track_stats = false;

    void on_node() noexcept {}
    void on_leaf() noexcept {}

    struct MoveLoop {
        void searched_move() noexcept {}
        void finish() noexcept {}
    };

    MoveLoop start_move_loop(Position&) {
        return {};
    }
};

struct BasicStatsObserver {
    static constexpr bool track_stats = true;

    std::uint64_t nodes = 0;
    std::uint64_t leaf_nodes = 0;

    void on_node() noexcept { nodes++; }
    void on_leaf() noexcept { leaf_nodes++; }

    struct MoveLoop {
        void searched_move() noexcept {}
        void finish() noexcept {}
    };

    MoveLoop start_move_loop(Position&) {
        return {};
    }
};

using PruningProfileHistogram =
    std::array<std::array<std::uint32_t, MovesList::capacity + 1>, MovesList::capacity + 1>;

struct PruningObserver {
    PruningProfileHistogram histogram{};

    void on_node() noexcept {}
    void on_leaf() noexcept {}

    struct MoveLoop {
        PruningProfileHistogram& histogram;
        std::size_t N;
        std::size_t k = 0;

        void searched_move();
        void finish();
    };

    MoveLoop start_move_loop(Position& position);
};

template <typename Observer>
SearchedMove pbm_fixed_depth_stats(Position& position, std::uint8_t depth, Observer& observer);

SearchedMove pbm_fixed_depth(Position& position, std::uint8_t depth);

SearchedMove pbm_fixed_time(Position& position, std::chrono::milliseconds thinking_time);

SearchedMove pbm_timed_game(
    Position& position,
    std::chrono::milliseconds remaining,
    std::chrono::milliseconds increment,
    std::optional<int> moves_to_go
);

std::vector<SearchedMove> rank_moves(Position& position, std::uint8_t depth);
