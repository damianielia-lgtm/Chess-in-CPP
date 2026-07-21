#pragma once
#include <string>
#include <cstdint>
#include <map>
#include <vector>
#include "../app/core/encoding.h"

uint64_t perft(Position& position, int depth);

struct expected_perft {
    std::string fen;
    std::string id;
    std::map<int, uint64_t> depths;
};

struct preset_info {
    uint64_t total_nodes = 0;
    uint64_t engine_nodes = 0;
    std::vector<expected_perft> positions{};
};
preset_info make_preset(std::string preset);

std::vector<expected_perft> epd_parser();

std::string run_perft(std::string preset, std::string mode);