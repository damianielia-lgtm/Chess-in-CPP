#pragma once

#include <cstdint>

#include "../core/position.h"

std::int16_t static_eval(const Position& position);
std::int16_t normalize_centipawn(std::int16_t eval);

constexpr std::uint8_t MAX_PHASE = 24;
std::uint8_t get_phase(const Position& position);
