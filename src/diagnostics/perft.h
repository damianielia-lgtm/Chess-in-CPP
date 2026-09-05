#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "../core/position.h"
#include "../storage/presets.h"

std::uint64_t perft(Position& position, int depth);
std::vector<std::string> run_test_preset(Preset preset);
std::vector<std::string> run_benchmark_preset(Preset preset);
std::vector<std::string> run_test(Position& position, int depth);
std::vector<std::string> run_benchmark(Position& position, int depth);
