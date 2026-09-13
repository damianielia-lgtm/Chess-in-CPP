#pragma once

#include <string>
#include <vector>
#include <cstdint>

#include "../core/position.h"
#include "../storage/presets.h"
#include "../notation/move_notation.h"
#include "../config.h"

std::vector<std::string> debug_pos(std::string fen, int depth);

std::vector<std::string> run_benchmark_engine(Position position, std::uint8_t depth, const ConfigData& config);
std::vector<std::string> benchmark_engine_preset(Preset preset, const ConfigData& config);

std::uint64_t perft(Position& position, int depth);
std::vector<std::string> run_test_preset(Preset preset);
std::vector<std::string> run_benchmark_preset(Preset preset);
std::vector<std::string> run_test(Position& position, int depth, MoveNotation notation);
std::vector<std::string> run_benchmark(Position& position, int depth);
