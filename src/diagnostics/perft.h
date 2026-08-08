#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include "../core/position.h"
#include "presets.h"

std::uint64_t perft(Position& position, int depth);
std::string run_test_preset(Preset preset);
std::string run_benchmark_preset(Preset preset);
std::string run_test(Position& position, int depth);
std::string run_benchmark(Position& position, int depth);
