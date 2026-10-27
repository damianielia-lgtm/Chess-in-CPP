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

using namespace std::chrono;

namespace {

constexpr std::int16_t INF = 32767;
constexpr std::int16_t MATE = 32766;

struct UnlimitedSearch {
    static constexpr bool expired = false;
    bool stop_soft() const noexcept { return false; }
    bool stop_hard() const noexcept { return false; }
};

struct TimedSearch {
    milliseconds soft_budget;
    milliseconds hard_budget;
    bool expired = false;
    steady_clock::time_point start = steady_clock::now();
    std::uint64_t node_counter = 0;

    bool stop_soft() noexcept {
        return expired || duration_cast<milliseconds>(steady_clock::now() - start) >= soft_budget;
    }

    bool stop_hard() noexcept {
        if (expired) { return true; }
        if ((node_counter++ & 2047) != 0) { return false; }
        expired = expired || duration_cast<milliseconds>(steady_clock::now() - start) >= hard_budget;
        return expired;
    }
};

std::int16_t terminal_eval(const Position& position, std::uint8_t ply) {
    const Color side = position.turn();
    const Color opponent =
        side == Color::White ? Color::Black : Color::White;

    const bool in_check = is_attacked_square(
        position, position.king_square(side), opponent
    );

    if (!in_check) {
        return 0;
    }

    return (MATE - ply) * (side == Color::White ? -1 : 1);
}

template <typename Observer, typename StopControl>
std::int16_t minimax_impl(
    Position& position,
    std::uint8_t depth,
    std::uint8_t ply,
    ScoredMoveListStack& move_lists,
    std::int16_t alpha,
    std::int16_t beta,
    Observer& observer,
    StopControl& control
) {
    observer.on_node();

    if (depth == 0) {
        observer.on_leaf();
        return static_eval(position);
    }

    if (control.stop_hard()) { return 0; }

    ScoredMoveList& legal_moves = move_lists[ply];
    generate_scored_moves(legal_moves, position);

    bool maximizing = position.turn() == Color::White;

    if (legal_moves.empty()) {
        observer.on_leaf();
        return terminal_eval(position, ply);
    }

    std::int16_t best_eval = maximizing ? -INF : INF;

    auto probe = observer.start_move_loop(legal_moves.size());

    while (!legal_moves.exhausted()) {
        Move move = legal_moves.next_move();

        UndoState move_state = position.apply_move(move);

        std::int16_t score = minimax_impl(
            position,
            depth - 1, ply + 1,
            move_lists,
            alpha, beta,
            observer, control
        );

        position.revert_move(move, move_state);

        if (control.expired) { return 0; }
        probe.searched_move();

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

    probe.finish();

    return best_eval;
}

template <typename Observer, typename StopControl>
SearchedMove pbm_impl(
    Position& position,
    std::uint8_t depth,
    Observer& observer,
    StopControl& control
) {
    assert(depth > 0);

    observer.on_node();

    bool maximizing = position.turn() == Color::White;
    SearchedMove search_result{std::nullopt, maximizing ? -INF : INF};

    ScoredMoveListStack move_lists;
    ScoredMoveList& legal_moves = move_lists[0];
    generate_scored_moves(legal_moves, position);
    
    if (legal_moves.empty()) {
        observer.on_leaf();
        search_result.eval = terminal_eval(position, 0);
        return search_result;
    }

    std::int16_t alpha = -INF;
    std::int16_t beta = INF;

    auto probe = observer.start_move_loop(legal_moves.size());

    while (!legal_moves.exhausted()) {
        Move move = legal_moves.next_move();
        
        UndoState move_state = position.apply_move(move);

        std::int16_t score = minimax_impl(
            position,
            depth - 1, 1,
            move_lists,
            alpha, beta,
            observer, control
        );

        position.revert_move(move, move_state);

        if (control.expired) { return search_result; }
        probe.searched_move();

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

    probe.finish();

    return search_result;
}

struct TimeLimits {
    milliseconds soft;
    milliseconds hard;
};

SearchedMove iterative_deepening(Position& position, TimeLimits limits) {
    TimedSearch control{limits.soft, limits.hard};

    MovesList legal_moves = all_moves(position);
    if (legal_moves.empty()) {
        return {std::nullopt, terminal_eval(position, 0)};
    }

    SearchedMove best{*legal_moves.begin(), static_eval(position)};
    NullObserver observer;

    for (std::uint8_t depth = 1; depth <= max_ply; depth++) {
        if (depth > 1 && control.stop_soft()) { break; }

        const SearchedMove candidate = pbm_impl(
            position, depth, observer, control
        );

        if (control.expired) { break; }
        best = candidate;
        if (abs(best.eval) >= MATE - depth) { break; }
    }

    return best;
}

}

std::vector<SearchedMove> rank_moves(Position& position, std::uint8_t depth) {
    assert(depth > 0);

    ScoredMoveListStack move_lists;
    MovesList legal_moves = all_moves(position);

    NullObserver observer;
    UnlimitedSearch control;
    std::vector<SearchedMove> ranked_moves;
    ranked_moves.reserve(legal_moves.size());

    for (const Move move : legal_moves) {
        UndoState move_state = position.apply_move(move);

        std::int16_t score = minimax_impl(
            position,
            depth - 1, 1,
            move_lists,
            -INF, INF,
            observer, control
        );

        position.revert_move(move, move_state);

        ranked_moves.push_back({move, normalize_centipawn(score)});
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
SearchedMove pbm_fixed_depth_stats(
    Position& position,
    std::uint8_t depth,
    Observer& observer
) {
    UnlimitedSearch control;
    SearchedMove raw_search = pbm_impl(position, depth, observer, control);
    raw_search.eval = normalize_centipawn(raw_search.eval);
    return raw_search;
}

template SearchedMove pbm_fixed_depth_stats<NullObserver>(Position& position, std::uint8_t depth, NullObserver& observer);
template SearchedMove pbm_fixed_depth_stats<BasicStatsObserver>(Position& position, std::uint8_t depth, BasicStatsObserver& observer);
template SearchedMove pbm_fixed_depth_stats<PruningObserver>(Position& position, std::uint8_t depth, PruningObserver& observer);

SearchedMove pbm_fixed_depth(Position& position, std::uint8_t depth) {
    NullObserver observer;
    UnlimitedSearch control;
    SearchedMove raw_search = pbm_impl(position, depth, observer, control);
    raw_search.eval = normalize_centipawn(raw_search.eval);
    return raw_search;
}

SearchedMove pbm_fixed_time(Position& position, milliseconds thinking_time) {
    SearchedMove raw_search = iterative_deepening(position, {thinking_time, thinking_time});
    raw_search.eval = normalize_centipawn(raw_search.eval);
    return raw_search;
}

SearchedMove pbm_timed_game(
    Position& position,
    milliseconds remaining,
    milliseconds increment,
    std::optional<int> moves_to_go
) {
    static constexpr milliseconds MaxThinking = milliseconds(40000); 

    remaining = std::max(remaining, milliseconds{0});
    increment = std::max(increment, milliseconds{0});

    const milliseconds reserve = std::min(milliseconds{500}, remaining / 5);
    const milliseconds available = remaining - reserve;

    const int estimated_moves = 20 + 20 * get_phase(position) / MAX_PHASE;
    const int horizon = moves_to_go
        ? std::max(1, *moves_to_go)
        : estimated_moves;

    const milliseconds soft = std::min({
        MaxThinking / 2,
        available,
        available / horizon + (increment / 4) * 3
    });
    const milliseconds hard = std::min({
        MaxThinking,
        available,
        soft * 3
    });

    SearchedMove raw_search = iterative_deepening(position, {soft, hard});
    raw_search.eval = normalize_centipawn(raw_search.eval);
    return raw_search;
}
