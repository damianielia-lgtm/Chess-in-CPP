#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include "../core/position.h"

uint64_t perft(Position& position, int depth);

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
PresetInfo make_preset(std::string preset);

std::vector<ExpectedPerft> epd_parser();

std::string run_perft(std::string preset, std::string mode);
