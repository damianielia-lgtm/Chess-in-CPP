#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include "../core/position.h"
#include "../interface/commands.h"

std::uint64_t perft(Position& position, int depth);
std::string run_perft(Preset preset, PerftMode mode);
