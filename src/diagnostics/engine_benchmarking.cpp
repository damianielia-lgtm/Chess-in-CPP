#include "engine_benchmarking.h"

#include <vector>
#include <string>
#include <cstdint>
#include <chrono>

#include "../core/position.h"
#include "../engine/search.h"
#include "../storage/presets.h"
#include "../interface/display.h"
#include "../notation/san.h"
#include "../config.h"

using namespace std::chrono;

std::chrono::milliseconds estimate_time(std::uint64_t total_nodes) {
    Position test_pos("startpos");
    auto start = steady_clock::now();
    pick_best_move(test_pos, 5);
    auto end = steady_clock::now();
    return duration_cast<milliseconds>((end - start) * total_nodes / 4865609);
}

std::vector<std::string> run_benchmark_engine(Position position, std::uint8_t depth) {
    auto start = steady_clock::now();
    SearchResult search_result = pick_best_move(position, depth);
    auto end = steady_clock::now();    
    duration<double> dur = end - start; 
    double speed = std::round(search_result.stats.nodes / dur.count() * 100.0) / 100.0;

    if (!search_result.best_move) { return {"No legal moves."}; }

    std::vector<std::string> lines;
    lines.push_back("Best move: " + search_result.best_move.value().to_uci());
    lines.push_back("Evaluation: " + std::to_string(search_result.eval) + " cp");
    lines.push_back("Nodes searched: " + std::to_string(search_result.stats.nodes));
    lines.push_back("Time: " + std::format("{:.2f}", dur.count()) + " s");
    lines.push_back("Raw Minimax speed: " + std::format("{}", speed) + " nodes/s");
    return lines;
}

std::vector<std::string> benchmark_engine_preset(Preset preset) {
    PresetInfo test_info;
    test_info = make_preset(preset);

    PerftProgress progress(test_info.total_nodes, estimate_time(test_info.total_nodes));

    std::vector<std::string> lines;
    lines.push_back("----- Engine Benchmark -- Preset " + preset_name(preset) + " -----");
    lines.push_back("");
    double total_dur = 0.0;
    std::uint64_t total_nodes = 0;
    for (ExpectedPerft test_state : test_info.positions) {
        lines.push_back("");
        lines.push_back("--- Running " + test_state.id + " - Fen: '" + test_state.fen + "' ---");
        lines.push_back("");
        Position pos(test_state.fen);
        
        for (const auto [depth, expected] : test_state.depths) {
            if (expected <= 5000) {
                lines.push_back("Depth " + std::to_string(depth) + ": Value too low to calculate speed reliably.");
                progress.advance(expected);
                continue;
            }

            auto start = steady_clock::now();
            SearchResult search_result = pick_best_move(pos, depth);
            auto end = steady_clock::now();
            duration<double> dur = end - start; 
            double speed = std::round(search_result.stats.nodes / dur.count() * 100.0) / 100.0;

            if (!search_result.best_move) { continue; }

            std::string line;
            line += "Depth " + std::to_string(depth) + ": ";
            line += search_result.best_move.value().to_uci() + " | ";
            line += std::to_string(search_result.eval) + " cp | ";
            line += std::to_string(search_result.stats.nodes) + " nodes | ";
            line += std::format("{:.2f}", dur.count()) + "s | ";
            line += std::format("{}", speed) + " nodes/sec";
            lines.push_back(line);

            total_dur += dur.count();
            total_nodes += search_result.stats.nodes;

            progress.advance(expected);
        }
    }

    lines.push_back("");
    lines.push_back("Total:");
    lines.push_back("Time: " + std::format("{:.2f}", total_dur) + 's');
    lines.push_back("Full tree node count: " + std::to_string(test_info.total_nodes));
    lines.push_back("Searched: " + std::to_string(total_nodes) + " nodes");
    lines.push_back("Raw Minimax Speed: " + std::format("{:.2f}", total_nodes / total_dur) + " nodes/s");
    lines.push_back("Pruning ratio: " + std::format("{:.2f}", 100.0 - (static_cast<double>(total_nodes) / test_info.total_nodes * 100.0)) + " %");
    lines.push_back("Perft-equivalent pruned Speed: " + std::format("{:.2f}", test_info.total_nodes / total_dur) + " nodes/s");

    return lines;
}
