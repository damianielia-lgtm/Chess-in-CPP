#pragma once

#include <vector>
#include <string>
#include <cstdint>

#include "../core/position.h"
#include "../storage/presets.h"
#include "../notation/move_notation.h"
#include "../config.h"

std::vector<std::string> run_benchmark_engine(Position position, std::uint8_t depth, const ConfigData& config);
std::vector<std::string> benchmark_engine_preset(Preset preset, const ConfigData& config);
