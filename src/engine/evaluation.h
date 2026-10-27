#pragma once

#include <cstdint>

#include "../core/position.h"

std::int16_t static_eval(const Position& position);
std::int16_t normalize_centipawn(std::int16_t eval);

constexpr std::uint8_t MAX_PHASE = 24;
std::uint8_t get_phase(const Position& position);

extern const std::array<std::array<std::int16_t, 15>, 15> mvvlva;
extern const std::array<std::array<std::array<std::int16_t, 64>, 15>, MAX_PHASE + 1> tampered_scores;
extern const std::array<std::array<std::int16_t, 4>, MAX_PHASE + 1> castling_scores;
