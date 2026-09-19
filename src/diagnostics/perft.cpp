#include "diagnostics.h"

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
#include "../interface/display.h"
#include "../storage/presets.h"
#include "../notation/move_notation.h"

using namespace std::chrono;

namespace {

std::uint64_t perft_impl(
    Position& position,
    std::uint8_t depth,
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

std::map<std::string, uint64_t> perft_div(Position& position, std::uint8_t depth, MoveNotation notation) {
    std::map<std::string, uint64_t> divide;
    MoveListStack move_lists;

    for (const Move move : all_moves(position, MoveGeneration::All)) {
        std::string move_string = move_notation(move, position, notation);
        UndoState move_state = position.apply_move(move);
        divide[move_string] = perft_impl(position, depth - 1, 0, move_lists);
        position.revert_move(move, move_state);
    }

    return divide;
}

void warmup() {
    Position test_pos("startpos");
    perft(test_pos, 5);
    perft(test_pos, 5);
    perft(test_pos, 5);
}

}

std::uint64_t perft(Position& position, std::uint8_t depth) {
    MoveListStack move_lists;
    return perft_impl(position, depth, 0, move_lists);
}

std::vector<std::string> perft_test(Position& position, std::uint8_t depth, MoveNotation notation) {
    std::vector<std::string> lines;
    std::uint64_t total_nodes = 0;
    for (const auto [move, nodes] : perft_div(position, depth, notation)) {
        lines.push_back(move + ": " + std::to_string(nodes));
        total_nodes += nodes;
    }
    lines.push_back("");
    lines.push_back("Total nodes: " + std::to_string(total_nodes));
    return lines;
}

std::vector<std::string> perft_benchmark(Position& position, std::uint8_t depth) {
    warmup();

    auto start = steady_clock::now();
    std::uint64_t nodes = perft(position, depth);
    auto end = steady_clock::now();    
    duration<double> dur = end - start; 
    double speed = std::round(nodes / dur.count() * 100.0) / 100.0;

    std::vector<std::string> lines;

    lines.push_back("Suite: \"" + position.to_fen() + "\" at depth " + std::to_string(depth));
    lines.push_back("Nodes: " + std::to_string(nodes));
    lines.push_back("Time: " + std::format("{:.2f} ms", dur.count() * 1000.0));
    lines.push_back("Speed: " + std::format("{}", speed) + " nodes/s");

    return lines;
}

std::vector<std::string> perft_test_preset(Preset preset) {
    PresetData test_info = make_preset(preset);

    ProgressDisplay progress(test_info.total_nodes);

    std::vector<std::string> lines;
    lines.push_back("----- Perft Test -- Preset " + preset_name(preset) + " -----");
    lines.push_back("");

    for (TestCase& suite : test_info.suites) {
        std::string suite_identifier = suite.id + " - D" + std::to_string(suite.depth);

        std::uint64_t nodes = perft(suite.position, suite.depth);

        if (nodes == suite.expected) {
            lines.push_back(suite_identifier + ": " + std::to_string(nodes) + " [PASS]");
        } else {
            lines.push_back(
                suite_identifier + ": " + std::to_string(nodes) + " [FAIL] " +
                "(expected: " + std::to_string(suite.expected) + ") " +
                "(fen: \"" + suite.position.to_fen() + "\")"
            );
        }

        progress.advance(suite.expected);
    }

    lines.push_back("");
    lines.push_back("--------------------------------");
    lines.push_back("");

    lines.push_back("Positions: " + std::to_string(test_info.positions_count));
    lines.push_back("Searches: " + std::to_string(test_info.suites.size()));

    return lines;
}

std::vector<std::string> perft_benchmark_preset(Preset preset) {
    PresetData test_info = make_preset(preset);
    warmup();

    ProgressDisplay progress(test_info.total_nodes);

    std::vector<std::string> lines;
    lines.push_back("----- Perft Benchmark -- Preset " + preset_name(preset) + " -----");
    lines.push_back("");

    double total_dur = 0.0;
    std::uint64_t total_nodes = 0;

    for (TestCase suite : test_info.suites) {
        std::string suite_identifier = suite.id + " - D" + std::to_string(suite.depth);

        auto start = steady_clock::now();
        std::uint64_t nodes = perft(suite.position, suite.depth);
        auto end = steady_clock::now();
        duration<double> dur = end - start; 
        double speed = std::round(nodes / dur.count() * 100.0) / 100.0;

        lines.push_back(
            suite_identifier + ": " +
            std::to_string(nodes) + " nodes | " +
            std::format("{:.2f} ms", dur.count() * 1000.0) + " | " +
            std::format("{}", speed) + " nodes/sec"
        );
        total_dur += dur.count();
        total_nodes += nodes;

        progress.advance(suite.expected);
    }

    lines.push_back("");
    lines.push_back("--------------------------------");
    lines.push_back("");

    lines.push_back("Positions: " + std::to_string(test_info.positions_count));
    lines.push_back("Searches: " + std::to_string(test_info.suites.size()));
    lines.push_back("Time: " + std::format("{:.2f} s", total_dur));
    lines.push_back("Nodes: " + std::to_string(total_nodes) + " nodes");
    lines.push_back("Speed: " + std::format("{:.2f}", total_nodes / total_dur) + " nodes/s");

    return lines;
}
