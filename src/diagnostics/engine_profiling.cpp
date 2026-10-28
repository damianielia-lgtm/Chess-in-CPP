#include "diagnostics.h"

#include <vector>
#include <string>
#include <cstdint>
#include <cstddef>
#include <cmath>
#include <sstream>
#include <iomanip>
#include <format>

#include "../core/position.h"
#include "../storage/presets.h"
#include "../engine/search.h"
#include "../interface/display.h"

PruningObserver::MoveLoop PruningObserver::start_move_loop(Position& position) {
    // The staged picker has no total size. Only profiling needs an eager legal count.
    const std::size_t legal_count = all_moves(position).size();
    assert(legal_count <= MovesList::capacity);
    return {histogram, legal_count};
}

void PruningObserver::MoveLoop::searched_move() {
    assert(k < N);
    k++;
}

void PruningObserver::MoveLoop::finish() {
    // Terminal nodes now reach the hooks, but have no move-loop observation.
    if (N == 0) { return; }
    assert(k > 0 && k <= N);
    histogram[N][k]++;
}

namespace {

void interpret_histogram(
    const PruningProfileHistogram& histogram,
    ReportContents& report
) {
    std::uint64_t total_observations = 0;

    for (std::size_t N = 1; N < histogram.size(); N++) {
        for (std::size_t k = 1; k <= N; k++) {
            total_observations += histogram[N][k];
        }
    }

    if (total_observations == 0) {
        report.text_lines.push_back("No move loop nodes searched.");
        return;
    }

    double average_N = 0;
    double average_k = 0;

    report.csv_lines = {"N,k,count"};

    for (std::size_t N = 1; N < histogram.size(); N++) {
        for (std::size_t k = 1; k <= N; k++) {
            std::uint32_t value = histogram[N][k];

            if (value != 0) {
                report.csv_lines.value().push_back(
                    std::to_string(N) + ',' + std::to_string(k) + ',' + std::to_string(value) 
                );
            }

            average_N += value * N;
            average_k += value * k;
        }
    }

    average_N /= total_observations;
    average_k /= total_observations;
    
    report.text_lines.push_back("Move-loop nodes: " + std::to_string(total_observations));
    report.text_lines.push_back("Average legal moves: " + std::format("{:.2f}", average_N));
    report.text_lines.push_back("Average moves searched: " + std::format("{:.2f}", average_k));
}

}

ReportContents profile_pruning(Position& position, std::uint8_t depth) {
    PruningObserver observer{};
    pbm_fixed_depth_stats<PruningObserver>(position, depth, observer);

    ReportContents report;

    report.text_lines.push_back("----- Pruning Profile -----");
    report.text_lines.push_back("");

    report.text_lines.push_back("Suite: \"" + position.to_fen() + "\" at depth " + std::to_string(depth));
    interpret_histogram(observer.histogram, report);

    return report;
}

ReportContents profile_pruning_preset(Preset preset) {
    PresetData test_info = make_preset(preset);
    ProgressDisplay progress(test_info.total_nodes);

    PruningObserver observer{};
    for (TestCase suite : test_info.suites) {
        pbm_fixed_depth_stats<PruningObserver>(suite.position, suite.depth, observer);
        progress.advance(suite.expected);
    }

    ReportContents report;

    report.text_lines.push_back("----- Pruning Profile -----");
    report.text_lines.push_back("");

    report.text_lines.push_back("Suite: " + preset_name(preset) + " preset");
    report.text_lines.push_back("Positions: " + std::to_string(test_info.positions_count));
    report.text_lines.push_back("Searches: " + std::to_string(test_info.suites.size()));

    interpret_histogram(observer.histogram, report);

    return report;
}
