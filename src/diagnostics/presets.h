#pragma once

#include <string>
#include <map>
#include <cstdint>
#include <vector>

struct ExpectedPerft {
    std::string fen;
    std::string id;
    std::map<int, uint64_t> depths;
};

struct PresetInfo {
    uint64_t total_nodes = 0;
    uint64_t engine_nodes = 0;
    std::vector<ExpectedPerft> positions{};
};

enum class Preset {
    Instant,
    Fast,
    Moderate,
    Extended
};

PresetInfo make_preset(Preset preset);