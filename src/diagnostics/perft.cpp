#include "perft.h"

#include <string>
#include <map>
#include <chrono>
#include <format>
#include <cmath>
#include <cstdint>
#include <vector>
#include <cassert>

#include "../movegen/legal_moves.h"
#include "../core/position.h"
#include "../core/move.h"
#include "../core/move_list.h"
#include "../interface/display.h"
#include "../storage/presets.h"

namespace {

std::uint64_t perft_impl(
    Position& position,
    int depth,
    std::size_t ply,
    MoveListStack& move_lists
) {
    assert(ply < max_ply);

    if (depth == 0) { return 1; }
    
    MovesList& legal_moves = move_lists[ply];
    generate_all_moves(legal_moves, position, MoveGeneration::All);
    
    if (depth == 1) { return legal_moves.size(); }

    std::uint64_t count = 0;
    for (const Move move : legal_moves) {
        UndoState move_state = position.apply_move(move);
        count += perft_impl(position, depth - 1, ply + 1, move_lists);
        position.revert_move(move, move_state);
    }

    return count;
}

std::map<std::string, uint64_t> perft_div(Position& position, int depth) {
    std::map<std::string, uint64_t> divide;
    MoveListStack move_lists;

    for (const Move move : all_moves(position, MoveGeneration::All)) {
        UndoState move_state = position.apply_move(move);
        divide[move.to_uci()] = perft_impl(position, depth - 1, 0, move_lists);
        position.revert_move(move, move_state);
    }

    return divide;
}

}

std::uint64_t perft(Position& position, int depth) {
    MoveListStack move_lists;
    return perft_impl(position, depth, 0, move_lists);
}

std::vector<std::string> run_test(Position& position, int depth) {
    std::vector<std::string> lines;
    std::uint64_t total_nodes = 0;
    for (const auto [move, nodes] : perft_div(position, depth)) {
        lines.push_back(move + ": " + std::to_string(nodes));
        total_nodes += nodes;
    }
    lines.push_back("");
    lines.push_back("Total nodes: " + std::to_string(total_nodes));
    return lines;
}

std::vector<std::string> run_benchmark(Position& position, int depth) {
    using namespace std::chrono;

    auto start = steady_clock::now();
    std::uint64_t nodes = perft(position, depth);
    auto end = steady_clock::now();    
    duration<double> dur = end - start; 
    double speed = std::round(nodes / dur.count() * 100.0) / 100.0;

    std::vector<std::string> lines;
    lines.push_back("Calculated " + std::to_string(nodes) + " nodes in " + std::format("{:.2f}", dur.count()) + " s");
    lines.push_back("Average speed " + std::format("{}", speed) + " nodes/s");
    return lines;
}

namespace {

std::chrono::milliseconds estimate_time(std::uint64_t total_nodes) {
    using namespace std::chrono;

    Position test_pos("startpos");
    auto start = steady_clock::now();
    perft(test_pos, 5);
    auto end = steady_clock::now();
    return duration_cast<milliseconds>((end - start) * total_nodes / 4865609);
}

}

std::vector<std::string> run_test_preset(Preset preset) {
    PresetInfo test_info;
    test_info = make_preset(preset);

    PerftProgress progress(test_info.total_nodes, estimate_time(test_info.total_nodes));

    std::vector<std::string> lines;
    lines.push_back("");
    lines.push_back("----- Perft Test -----");
    lines.push_back("");
    for (ExpectedPerft& perft_state : test_info.positions) {
        lines.push_back("");
        lines.push_back("--- Running " + perft_state.id + " - Fen: '" + perft_state.fen + "' ---");
        lines.push_back("");
        Position pos(perft_state.fen);
        
        for (const auto [depth, expected] : perft_state.depths) {
            std::uint64_t nodes = perft(pos, depth);

            if (nodes == expected) {
                lines.push_back("Depth " + std::to_string(depth) + ": " + std::to_string(nodes) + " [PASS]");
            } else {
                lines.push_back(
                    "Depth " + std::to_string(depth) + ": " + std::to_string(nodes) +
                    " [FAIL] (expected: " + std::to_string(expected) + ")"
                );
            }

            progress.advance(expected);
        }
    }

    lines.push_back("");

    return lines;
}

std::vector<std::string> run_benchmark_preset(Preset preset) {
    using namespace std::chrono;

    PresetInfo test_info;
    test_info = make_preset(preset);

    PerftProgress progress(test_info.total_nodes, estimate_time(test_info.total_nodes));

    std::vector<std::string> lines;
    lines.push_back("");
    lines.push_back("----- Perft Benchmark -----");
    lines.push_back("");
    double total_dur = 0.0;
    std::uint64_t total_nodes = 0;
    for (ExpectedPerft perft_state : test_info.positions) {
        lines.push_back("");
        lines.push_back("--- Running " + perft_state.id + " - Fen: '" + perft_state.fen + "' ---");
        lines.push_back("");
        Position pos(perft_state.fen);
        
        for (const auto [depth, expected] : perft_state.depths) {
            if (expected <= 5000) {
                lines.push_back("Depth " + std::to_string(depth) + ": Value too low to calculate speed reliably.");
                progress.advance(expected);
                continue;
            }

            auto start = steady_clock::now();
            std::uint64_t nodes = perft(pos, depth);
            auto end = steady_clock::now();
            duration<double> dur = end - start; 
            double speed = std::round(nodes / dur.count() * 100.0) / 100.0;

            lines.push_back(
                "Depth " + std::to_string(depth) + ": " +
                std::to_string(nodes) + " nodes in " + std::format("{:.2f}", dur.count()) + "s - " +
                std::format("{}", speed) + " nodes/sec"
            );
            total_dur += dur.count();
            total_nodes += nodes;

            progress.advance(expected);
        }
    }

    lines.push_back("");
    lines.push_back(
        "Total: " + std::to_string(total_nodes) + " nodes in " + std::format("{:.2f}", total_dur) +
        "s - Average Speed: " + std::format("{:.2f}", total_nodes / total_dur) + " nodes/s"
    );

    return lines;
}
