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

namespace {

constexpr std::int16_t INF = 32767;
constexpr std::int16_t MATE = 32766;

std::int16_t terminal_eval(const Position& position) {
    const Color side = position.turn();
    const Color opponent =
        side == Color::White ? Color::Black : Color::White;

    const bool in_check = is_attacked_square(
        position, position.king_square(side), opponent
    );

    if (!in_check) {
        return 0;
    }

    return side == Color::White ? -MATE : MATE;
}

template <typename Observer>
std::int16_t minimax_impl(
    Position& position,
    std::uint8_t depth,
    std::uint8_t ply,
    MoveListStack& move_lists,
    std::int16_t alpha,
    std::int16_t beta,
    Observer& observer
) {
    observer.on_node();

    if (depth == 0) {
        observer.on_leaf();
        return static_eval(position);
    }

    MovesList& legal_moves = move_lists[ply];
    generate_all_moves(legal_moves, position, MoveGeneration::All);

    bool maximizing = position.turn() == Color::White;

    if (legal_moves.empty()) {
        observer.on_leaf();
        return terminal_eval(position);
    }

    std::int16_t best_eval = maximizing ? -INF : INF;

    for (const Move move : legal_moves) {
        UndoState move_state = position.apply_move(move);
        std::int16_t score =
            minimax_impl(position, depth - 1, ply + 1, move_lists, alpha, beta, observer);
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

}

std::int16_t minimax(Position& position, std::uint8_t depth) {
    MoveListStack move_lists;
    NullObserver observer;
    std::int16_t eval =
        minimax_impl(position, depth, 0, move_lists, -INF, INF, observer);
    return eval;
}

std::vector<SearchedMove> rank_moves(Position& position, std::uint8_t depth) {
    assert(depth > 0);

    MoveListStack move_lists;
    MovesList& legal_moves = move_lists[0];
    generate_all_moves(legal_moves, position, MoveGeneration::All);

    NullObserver observer;
    std::vector<SearchedMove> ranked_moves;
    ranked_moves.reserve(legal_moves.size());

    for (const Move move : legal_moves) {
        UndoState move_state = position.apply_move(move);
        std::int16_t score =
            minimax_impl(position, depth - 1, 1, move_lists, -INF, INF, observer);
        position.revert_move(move, move_state);

        ranked_moves.push_back({move, score});
    }

    bool maximizing = position.turn() == Color::White;
    std::sort(
        ranked_moves.begin(),
        ranked_moves.end(),
        [maximizing](const SearchedMove& a, const SearchedMove& b) {
            return maximizing ? a.eval > b.eval : a.eval < b.eval;
        }
    );

    return ranked_moves;
}

template <typename Observer>
SearchedMove pick_best_move(
    Position& position,
    std::uint8_t depth,
    Observer& observer
) {
    assert(depth > 0);

    bool maximizing = position.turn() == Color::White;
    SearchedMove search_result{std::nullopt, maximizing ? -INF : INF};

    MoveListStack move_lists;
    MovesList& legal_moves = move_lists[0];
    generate_all_moves(legal_moves, position, MoveGeneration::All);
    
    if (legal_moves.empty()) {
        search_result.eval = terminal_eval(position);
        return search_result;
    }

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
            observer
        );

        position.revert_move(move, move_state);

        if (
            !search_result.move ||
            (maximizing ? score > search_result.eval : score < search_result.eval)
        ) {
            search_result.move = move;
            search_result.eval = score;
        }

        if (maximizing) {
            alpha = std::max(alpha, search_result.eval);
        } else {
            beta = std::min(beta, search_result.eval);
        }
    }

    return search_result;
}

template SearchedMove pick_best_move<NullObserver>(Position& position, std::uint8_t depth, NullObserver& observer);
template SearchedMove pick_best_move<BasicStatsObserver>(Position& position, std::uint8_t depth, BasicStatsObserver& observer);

SearchedMove pick_best_move(Position& position, std::uint8_t depth) {
    NullObserver observer;
    return pick_best_move(position, depth, observer);
}
