#pragma once

#include <vector>
#include <string>
#include <cstdint>

#include "../core/position.h"
#include "../storage/presets.h"

std::vector<std::string> run_benchmark_engine(Position position, std::uint8_t depth);
std::vector<std::string> benchmark_engine_preset(Preset preset);
