#pragma once

#include <cstdint>

#include "../core/position.h"

constexpr std::uint8_t MAX_PHASE = 24;

struct StaticEval {
    std::uint8_t phase;
    std::int16_t mg_eval;
    std::int16_t eg_eval;
};
StaticEval get_static_eval(const Position& position);

std::int16_t normalize_centipawn(std::int16_t eval);

extern const std::array<std::array<std::int16_t, 15>, 15> mvvlva;
extern const std::array<std::array<std::array<std::int16_t, 64>, 15>, MAX_PHASE + 1> tampered_scores;
extern const std::array<std::array<std::int16_t, 4>, MAX_PHASE + 1> castling_scores;
