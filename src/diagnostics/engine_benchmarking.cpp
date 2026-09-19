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

void warmup() {
    Position test_pos("startpos");
    pick_best_move(test_pos, 6);
    pick_best_move(test_pos, 6);
    pick_best_move(test_pos, 6);
}

template <typename Observer>
std::vector<std::string> benchmark_engine_impl(
    Position position,
    std::uint8_t depth,
    const ConfigData& config
) {
    warmup();
    Observer observer{};

    auto start = steady_clock::now();
    SearchedMove search_result = pick_best_move(position, depth, observer);
    auto end = steady_clock::now();    
    duration<double> dur = end - start; 

    if (!search_result.move) {
        return std::vector<std::string>{"No legal moves."};
    }

    std::vector<std::string> lines;

    lines.push_back("Suite: \"" + position.to_fen() + "\" at depth " + std::to_string(depth));
    lines.push_back("Best move: " + move_notation(*search_result.move, position, config.move_notation));
    lines.push_back("Evaluation: " + std::to_string(search_result.eval) + " cp");
    lines.push_back("Time: " + std::format("{:.2f} ms", dur.count() * 1000.0));
    lines.push_back("");

    if constexpr (Observer::track_stats) {
        lines.push_back("Tracked:");
        lines.push_back("Nodes: " + std::to_string(observer.nodes));
        lines.push_back("Raw NPS: " + std::format("{}", observer.nodes / static_cast<double>(dur.count())) + " nodes/s");
    }

    return lines;
}

template <typename Observer>
std::vector<std::string> benchmark_engine_preset_impl(Preset preset, const ConfigData& config) {
    PresetData test_info = make_preset(preset);
    warmup();

    ProgressDisplay progress(test_info.total_nodes);

    std::vector<std::string> lines;
    lines.push_back("----- Engine Benchmark -- Preset " + preset_name(preset) + " -----");
    lines.push_back("");

    double total_dur = 0.0;
    std::uint64_t total_nodes = 0;
    std::uint64_t total_leaf_nodes = 0;

    for (TestCase suite : test_info.suites) {
        std::string suite_identifier = suite.id + " - D" + std::to_string(suite.depth);

        Observer observer{};

        auto start = steady_clock::now();
        SearchedMove search_result = pick_best_move(suite.position, suite.depth, observer);
        auto end = steady_clock::now();
        duration<double> dur = end - start; 

        if (!search_result.move) {
            progress.advance(suite.expected);
            continue;
        }

        std::string line;

        line += suite_identifier + ": ";
        line += move_notation(*search_result.move, suite.position, config.move_notation) + " | ";
        line += std::to_string(search_result.eval) + " cp | ";
        line += std::format("{:.2f} ms", dur.count() * 1000.0);

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

        progress.advance(suite.expected);
    }

    lines.push_back("");
    lines.push_back("--------------------------------");
    lines.push_back("");

    lines.push_back("Positions: " + std::to_string(test_info.positions_count));
    lines.push_back("Searches: " + std::to_string(test_info.suites.size()));
    lines.push_back("Time: " + std::format("{:.2f} s", total_dur));
    lines.push_back("");

    if constexpr (Observer::track_stats) {
        lines.push_back("Tracked: ");
        lines.push_back("Nodes: " + std::to_string(total_nodes) + " nodes");
        lines.push_back("Leaves: " + std::to_string(total_leaf_nodes) + " nodes");
        lines.push_back("Raw NPS: " + std::format("{:.2f}", total_nodes / total_dur) + " nodes/s");
        lines.push_back(
            "Pruning ratio: " + std::format(
                "{:.2f}", 100.0 - (static_cast<double>(total_leaf_nodes) / test_info.total_nodes * 100.0)
            ) + " %"
        );
        lines.push_back("");
    }

    lines.push_back("Full tree node count: " + std::to_string(test_info.total_nodes));
    lines.push_back("Effective full-tree rate: " + std::format("{:.2f}", test_info.total_nodes / total_dur) + " nodes/s");

    return lines;
}

};

std::vector<std::string> benchmark_engine(
    Position position,
    std::uint8_t depth,
    const ConfigData& config
) {
    return config.track_stats
        ? benchmark_engine_impl<BasicStatsObserver>(position, depth, config)
        : benchmark_engine_impl<NullObserver>(position, depth, config);
}

std::vector<std::string> benchmark_engine_preset(Preset preset, const ConfigData& config) {
    return config.track_stats
        ? benchmark_engine_preset_impl<BasicStatsObserver>(preset, config)
        : benchmark_engine_preset_impl<NullObserver>(preset, config);
}
