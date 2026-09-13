#include "diagnostics.h"

#include <vector>
#include <string>
#include <cstdint>
#include <chrono>
#include <cmath>

#include "../core/position.h"
#include "../engine/search.h"
#include "../storage/presets.h"
#include "../interface/display.h"
#include "../notation/move_notation.h"
#include "../config.h"

namespace {

using namespace std::chrono;

std::chrono::milliseconds estimate_time(std::uint64_t total_nodes) {
    Position test_pos("startpos");

    auto start = steady_clock::now();
    pick_best_move(test_pos, 5);
    auto end = steady_clock::now();

    return duration_cast<milliseconds>((end - start) * total_nodes / 4865609);
}

template <typename Observer>
std::vector<std::string> run_benchmark_engine_impl(
    Position position,
    std::uint8_t depth,
    const ConfigData& config
) {
    Observer observer{};

    auto start = steady_clock::now();
    SearchedMove search_result = pick_best_move(position, depth, observer);
    auto end = steady_clock::now();    
    duration<double> dur = end - start; 

    if (!search_result.move) {
        return std::vector<std::string>{"No legal moves."};
    }

    std::vector<std::string> lines;

    lines.push_back("Best move: " + move_notation(*search_result.move, position, config.move_notation));
    lines.push_back("Evaluation: " + std::to_string(search_result.eval) + " cp");
    lines.push_back("Time: " + std::format("{:.2f}", dur.count()) + " s");

    if constexpr (Observer::track_stats) {
        lines.push_back("Nodes searched: " + std::to_string(observer.nodes));
        double speed = std::round(observer.nodes / dur.count() * 100.0) / 100.0;
        lines.push_back("Raw Minimax speed: " + std::format("{}", speed) + " nodes/s");
    }

    return lines;
}

template <typename Observer>
std::vector<std::string> benchmark_engine_preset_impl(Preset preset, const ConfigData& config) {
    PresetInfo test_info;
    test_info = make_preset(preset);

    ProgressDisplay progress(test_info.total_nodes, estimate_time(test_info.total_nodes));

    std::vector<std::string> lines;
    lines.push_back("----- Engine Benchmark -- Preset " + preset_name(preset) + " -----");
    lines.push_back("");

    double total_dur = 0.0;
    std::uint64_t total_nodes = 0;
    std::uint64_t total_leaf_nodes = 0;

    for (ExpectedPerft test_state : test_info.positions) {
        lines.push_back("");
        lines.push_back("--- Running " + test_state.id + " - Fen: '" + test_state.fen + "' ---");
        lines.push_back("");
        Position position(test_state.fen);
        
        for (const auto [depth, expected] : test_state.depths) {
            if (expected <= 5000) {
                lines.push_back("Depth " + std::to_string(depth) + ": Value too low to calculate speed reliably.");
                progress.advance(expected);
                continue;
            }

            Observer observer{};

            auto start = steady_clock::now();
            SearchedMove search_result = pick_best_move(position, depth, observer);
            auto end = steady_clock::now();
            duration<double> dur = end - start; 

            if (!search_result.move) {
                progress.advance(expected);
                continue;
            }

            std::string line;

            line += "Depth " + std::to_string(depth) + ": ";
            line += move_notation(*search_result.move, position, config.move_notation) + " | ";
            line += std::to_string(search_result.eval) + " cp | ";
            line += std::format("{:.2f}", dur.count()) + "s";

            if constexpr (Observer::track_stats) {
                line += " | ";
                line += std::to_string(observer.nodes) + " nodes | ";

                double speed = std::round(observer.nodes / dur.count() * 100.0) / 100.0;
                line += std::format("{}", speed) + " nodes/sec";

                total_nodes += observer.nodes;
                total_leaf_nodes += observer.leaf_nodes;
            }

            lines.push_back(line);

            total_dur += dur.count();

            progress.advance(expected);
        }
    }

    lines.push_back("");
    lines.push_back("Total:");

    lines.push_back("Time: " + std::format("{:.2f}", total_dur) + 's');
    lines.push_back("Full tree node count: " + std::to_string(test_info.total_nodes));
    lines.push_back("Perft-equivalent pruned Speed: " + std::format("{:.2f}", test_info.total_nodes / total_dur) + " nodes/s");

    if constexpr (Observer::track_stats) {
        lines.push_back("Searched: " + std::to_string(total_nodes) + " nodes");
        lines.push_back("Raw Minimax Speed: " + std::format("{:.2f}", total_nodes / total_dur) + " nodes/s");
        lines.push_back(
            "Pruning ratio: " + std::format(
                "{:.2f}", 100.0 - (static_cast<double>(total_leaf_nodes) / test_info.total_nodes * 100.0)
            ) + " %"
        );
    }

    return lines;
}

};

std::vector<std::string> run_benchmark_engine(
    Position position,
    std::uint8_t depth,
    const ConfigData& config
) {
    return config.track_stats
        ? run_benchmark_engine_impl<BasicStatsObserver>(position, depth, config)
        : run_benchmark_engine_impl<NullObserver>(position, depth, config);
}

std::vector<std::string> benchmark_engine_preset(Preset preset, const ConfigData& config) {
    return config.track_stats
        ? benchmark_engine_preset_impl<BasicStatsObserver>(preset, config)
        : benchmark_engine_preset_impl<NullObserver>(preset, config);
}
