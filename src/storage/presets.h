#pragma once

#include <string>
#include <map>
#include <cstdint>
#include <vector>

#include "../core/position.h"

struct TestCase {
    Position position;
    std::string id;
    std::uint8_t depth;
    std::uint64_t expected;
};

struct PresetData {
    uint64_t total_nodes = 0;
    int positions_count = 0;
    std::vector<TestCase> suites;
};

enum class Preset {
    Fast,
    Moderate,
    Extended
};

PresetData make_preset(Preset preset);
std::string preset_name(Preset preset);
