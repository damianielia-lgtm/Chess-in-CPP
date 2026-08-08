#include "perft.h"

#include <string>
#include <map>
#include <chrono>
#include <format>
#include <cmath>
#include <cstdint>
#include <iostream>

#include "../movegen/legal_moves.h"
#include "../core/position.h"
#include "../core/move.h"
#include "../core/move_list.h"
#include "../interface/display.h"
#include "presets.h"

std::uint64_t perft_impl(
    Position& position,
    int depth,
    std::size_t ply,
    MoveListStack& move_lists
) {
    assert(ply < max_ply);

    if (depth == 0) { return 1; }
    
    MovesList& legal_moves = move_lists[ply];
    legal_moves.clear();
    generate_all_moves(legal_moves, position, false);
    
    if (depth == 1) { return legal_moves.size(); }

    std::uint64_t count = 0;
    for (const Move move : legal_moves) {
        UndoState move_state = position.apply_move(move);
        count += perft_impl(position, depth - 1, ply + 1, move_lists);
        position.revert_move(move, move_state);
    }

    return count;
}

std::uint64_t perft(Position& position, int depth) {
    MoveListStack move_lists;
    return perft_impl(position, depth, 0, move_lists);
}

std::map<std::string, uint64_t> perft_div(Position& position, int depth) {
    std::map<std::string, uint64_t> divide;
    MoveListStack move_lists;

    for (const Move move : all_moves(position, false)) {
        UndoState move_state = position.apply_move(move);
        divide[move.to_uci()] = perft_impl(position, depth - 1, 0, move_lists);
        position.revert_move(move, move_state);
    }

    return divide;
}

std::string run_test(Position& position, int depth) {
    std::string lines;
    std::uint64_t total_nodes = 0;
    for (const auto [move, nodes] : perft_div(position, depth)) {
        lines += move + ": " + std::to_string(nodes) + '\n';
        total_nodes += nodes;
    }
    lines += "\nTotal nodes: " + std::to_string(total_nodes) + '\n';
    return lines;
}

std::string run_benchmark(Position& position, int depth) {
    using namespace std::chrono;

    auto start = steady_clock::now();
    std::uint64_t nodes = perft(position, depth);
    auto end = steady_clock::now();    
    duration<double> dur = end - start; 
    double speed = std::round(nodes / dur.count() * 100.0) / 100.0;

    std::string lines;
    lines += "Calculated " + std::to_string(nodes) + " nodes in " + std::format("{:.2f}", dur.count()) + " s\n";
    lines += "Average speed " + std::format("{}", speed) + " nodes/s\n"; 
    return lines;
}

std::chrono::milliseconds estimate_time(std::uint64_t total_nodes) {
    using namespace std::chrono;

    Position test_pos("startpos");
    auto start = steady_clock::now();
    perft(test_pos, 5);
    auto end = steady_clock::now();
    return duration_cast<milliseconds>((end - start) * total_nodes / 4865609);
}

std::string run_test_preset(Preset preset) {
    PresetInfo test_info;
    test_info = make_preset(preset);

    PerftProgress progress(test_info.total_nodes, estimate_time(test_info.total_nodes));

    std::string lines;
    lines += "\n----- Perft Test -----\n\n";
    for (ExpectedPerft perft_state : test_info.positions) {
        lines += "\n--- Running " + perft_state.id + " - Fen: '" + perft_state.fen + "' ---\n\n";
        Position pos(perft_state.fen);
        
        for (const auto [depth, expected] : perft_state.depths) {
            std::uint64_t nodes = perft(pos, depth);

            if (nodes == expected) {
                lines += "Depth " + std::to_string(depth) + ": " + std::to_string(nodes) + " [PASS]\n";
            } else {
                lines += "Depth " + std::to_string(depth) + ": " + std::to_string(nodes);
                lines += " [FAIL] (expected: " + std::to_string(expected) + ")\n";
            }

            progress.advance(expected);
        }
    }

    lines += '\n';

    return lines;
}

std::string run_benchmark_preset(Preset preset) {
    using namespace std::chrono;

    PresetInfo test_info;
    test_info = make_preset(preset);

    PerftProgress progress(test_info.total_nodes, estimate_time(test_info.total_nodes));

    std::string lines;
    lines += "\n----- Perft Benchmark -----\n\n";
    double total_dur = 0.0;
    std::uint64_t total_nodes = 0;
    for (ExpectedPerft perft_state : test_info.positions) {
        lines += "\n--- Running " + perft_state.id + " - Fen: '" + perft_state.fen + "' ---\n\n";
        Position pos(perft_state.fen);
        
        for (const auto [depth, expected] : perft_state.depths) {
            if (expected <= 5000) {
                lines += "Depth " + std::to_string(depth) + ": Value too low to calculate speed reliably.\n";
                progress.advance(expected);
                continue;
            }

            auto start = steady_clock::now();
            std::uint64_t nodes = perft(pos, depth);
            auto end = steady_clock::now();
            duration<double> dur = end - start; 
            double speed = std::round(nodes / dur.count() * 100.0) / 100.0;

            lines += "Depth " + std::to_string(depth) + ": ";
            lines += std::to_string(nodes) + " nodes in " + std::format("{:.2f}", dur.count()) + "s - ";
            lines += std::format("{}", speed) + " nodes/sec\n";
            total_dur += dur.count();
            total_nodes += nodes;

            progress.advance(expected);
        }
    }

    lines += '\n';
    lines += "Total: " + std::to_string(total_nodes) + " nodes in " + std::format("{:.2f}", total_dur);
    lines += "s - Average Speed: " + std::format("{:.2f}", total_nodes / total_dur) + " nodes/s\n";

    return lines;
}
