#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <optional>

#include "../core/position.h"
#include "../storage/presets.h"
#include "../notation/move_notation.h"
#include "../config.h"

struct ReportContents {
    std::vector<std::string> text_lines;
    std::optional<std::vector<std::string>> csv_lines;
};

ReportContents debug_pos(std::string fen, std::uint8_t depth);

ReportContents benchmark_engine(Position position, std::uint8_t depth, const ConfigData& config);
ReportContents benchmark_engine_preset(Preset preset, const ConfigData& config);

std::uint64_t perft(Position& position, std::uint8_t depth);
ReportContents perft_test_preset(Preset preset);
ReportContents perft_benchmark_preset(Preset preset);
ReportContents perft_test(Position& position, std::uint8_t depth, MoveNotation notation);
ReportContents perft_benchmark(Position& position, std::uint8_t depth);

ReportContents profile_pruning(Position& position, std::uint8_t depth);
ReportContents profile_pruning_preset(Preset preset);
