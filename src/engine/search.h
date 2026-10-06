#pragma once

#include <cstdint>
#include <optional>
#include <vector>
#include <array>
#include <cassert>

#include "../core/position.h"
#include "../core/move.h"
#include "../movegen/legal_moves.h"

std::int16_t minimax(Position& position, std::uint8_t depth);

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

    MoveLoop start_move_loop(std::size_t) {
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

    MoveLoop start_move_loop(std::size_t) {
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

        void searched_move() {
            assert(k < N);
            k++;
        }

        void finish() {
            histogram[N][k]++;
        }
    };

    MoveLoop start_move_loop(std::size_t list_size) {
        assert(list_size > 0 && list_size <= MovesList::capacity);
        return {histogram, list_size};
    }
};

template <typename Observer>
SearchedMove pick_best_move(Position& position, std::uint8_t depth, Observer& observer);

SearchedMove pick_best_move(Position& position, std::uint8_t depth);
std::vector<SearchedMove> rank_moves(Position& position, std::uint8_t depth);
