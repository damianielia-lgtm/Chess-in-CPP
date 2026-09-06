#include "search.h"

#include <cstdint>
#include <algorithm>
#include <optional>
#include <cassert>
#include <vector>

#include "../core/position.h"
#include "../core/move.h"
#include "../movegen/legal_moves.h"
#include "../movegen/attacks.h"
#include "evaluation.h"

constexpr std::int16_t INF = 32767;
constexpr std::int16_t MATE = 32766;

std::int16_t minimax_impl(
    Position& position,
    std::uint8_t depth,
    std::uint8_t ply,
    MoveListStack& move_lists,
    std::int16_t alpha,
    std::int16_t beta,
    std::uint64_t& nodes
) {
    ++nodes;

    if (depth == 0) {
        return static_eval(position);
    }

    MovesList& legal_moves = move_lists[ply];
    generate_all_moves(legal_moves, position, MoveGeneration::All);

    bool maximizing = position.turn() == Color::White;

    if (legal_moves.empty()) {
        return (
            maximizing
                ? is_attacked_square(position, position.king_square(Color::White), Color::Black)
                    ? -MATE
                    : 0
                : is_attacked_square(position, position.king_square(Color::Black), Color::White)
                    ? MATE
                    : 0
        );
    }

    std::int16_t best_eval = maximizing ? -INF : INF;

    for (const Move move : legal_moves) {
        UndoState move_state = position.apply_move(move);
        std::int16_t score = minimax_impl(position, depth - 1, ply + 1, move_lists, alpha, beta, nodes);
        position.revert_move(move, move_state);

        if (maximizing) {
            best_eval = std::max(best_eval, score);
            alpha = std::max(alpha, best_eval);
        } else {
            best_eval = std::min(best_eval, score);
            beta = std::min(beta, best_eval);
        }

        if (beta <= alpha) {
            break;
        }
    }

    return best_eval;
}

std::int16_t minimax(Position& position, std::uint8_t depth) {
    MoveListStack move_lists;
    std::uint64_t nodes;
    std::int16_t eval = minimax_impl(position, depth, 0, move_lists, -INF, INF, nodes);
    return normalize_centipawn(eval);
}

std::vector<RankedMove> rank_moves(Position& position, std::uint8_t depth) {
    assert(depth > 0);

    MoveListStack move_lists;
    MovesList& legal_moves = move_lists[0];
    generate_all_moves(legal_moves, position, MoveGeneration::All);

    std::uint64_t nodes;
    std::vector<RankedMove> ranked_moves;
    ranked_moves.reserve(legal_moves.size());

    for (const Move move : legal_moves) {
        UndoState move_state = position.apply_move(move);
        std::int16_t score = minimax_impl(position, depth - 1, 1, move_lists, -INF, INF, nodes);
        position.revert_move(move, move_state);

        ranked_moves.push_back({move, score});
    }

    bool maximizing = position.turn() == Color::White;
    std::sort(ranked_moves.begin(), ranked_moves.end(), [maximizing](const RankedMove& a, const RankedMove& b) {
        return maximizing ? a.eval > b.eval : a.eval < b.eval;
    });

    return ranked_moves;
}

SearchResult pick_best_move(Position& position, std::uint8_t depth) {
    assert(depth > 0);

    bool maximizing = position.turn() == Color::White;
    SearchResult search_result{std::nullopt, maximizing ? -INF : INF, SearchStats{}};

    MoveListStack move_lists;
    MovesList& legal_moves = move_lists[0];
    generate_all_moves(legal_moves, position, MoveGeneration::All);
    
    if (legal_moves.empty()) { return search_result; }

    std::int16_t alpha = -INF;
    std::int16_t beta = INF;

    for (const Move move : legal_moves) {
        UndoState move_state = position.apply_move(move);

        std::int16_t score = minimax_impl(
            position,
            depth - 1,
            1,
            move_lists,
            alpha,
            beta,
            search_result.stats.nodes
        );

        position.revert_move(move, move_state);

        if (!search_result.best_move) { search_result.best_move = move; }

        if (maximizing) {
            if (score > search_result.eval) { search_result.best_move = move; }
            search_result.eval = std::max(search_result.eval, score);
            alpha = std::max(alpha, search_result.eval);
        } else {
            if (score < search_result.eval) { search_result.best_move = move; }
            search_result.eval = std::min(search_result.eval, score);
            beta = std::min(beta, search_result.eval);
        }
    }

    search_result.eval = normalize_centipawn(search_result.eval);

    return search_result;
}
