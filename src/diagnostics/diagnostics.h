#pragma once

#include <string>
#include <vector>
#include <cstdint>

#include "../core/position.h"
#include "../storage/presets.h"
#include "../notation/move_notation.h"
#include "../config.h"

std::vector<std::string> debug_pos(std::string fen, std::uint8_t depth);

std::vector<std::string> benchmark_engine(Position position, std::uint8_t depth, const ConfigData& config);
std::vector<std::string> benchmark_engine_preset(Preset preset, const ConfigData& config);

std::uint64_t perft(Position& position, std::uint8_t depth);
std::vector<std::string> perft_test_preset(Preset preset);
std::vector<std::string> perft_benchmark_preset(Preset preset);
std::vector<std::string> perft_test(Position& position, std::uint8_t depth, MoveNotation notation);
std::vector<std::string> perft_benchmark(Position& position, std::uint8_t depth);
