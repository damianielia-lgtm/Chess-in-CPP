#include "presets.h"

#include <string>
#include <fstream>
#include <vector>
#include <sstream>
#include <map>
#include <cstdint>

#include "../core/position.h"
#include "../errors.h"

namespace {

std::vector<ExpectedPerft> epd_parser() {
    std::ifstream file("resources/diagnostics/perft_database.epd");
    if (!file) {
        throw StorageIoError("Could not load perft database");
    }

    std::vector<ExpectedPerft> pos_list;
    std::string line;

    while (std::getline(file, line)) {
        ExpectedPerft pos;

        std::vector<std::string> tokens;
        std::stringstream ss(line);
        std::string token;
        while (std::getline(ss, token, ';')) {
            if (token.empty()) {continue;}
            if (token[0] == ' ') {
                token.erase(0, 1);
            }
            tokens.push_back(token);
        }

        pos.fen = tokens[0];

        for (size_t operator_index = 1; operator_index < tokens.size(); operator_index++) {
            std::string field = tokens[operator_index];
            if (field.starts_with("id")) {
                pos.id = field.substr(3);
                pos.id.erase(0, 1);
                pos.id.pop_back();
            } else if (field.starts_with("D")) {
                int depth = std::stoi(field.substr(1, field.find(' ') - 1));
                std::uint64_t count = std::stoull(field.substr(field.find(' ') + 1));
                pos.depths[depth] = count;
            }
        }

        pos_list.push_back(pos);
    }

    return pos_list;
}

const std::map<std::string, std::uint64_t> preset_max_nodes = {
    {"instant", 300000},
    {"fast", 1000000},
    {"moderate", 5000000},
    {"extended", 20000000}
};

}

PresetInfo make_preset(Preset preset) {
    PresetInfo info;
    std::uint64_t max_nodes;
    switch (preset) {
        case Preset::Instant: max_nodes = preset_max_nodes.at("instant"); break;
        case Preset::Fast: max_nodes = preset_max_nodes.at("fast"); break;
        case Preset::Moderate: max_nodes = preset_max_nodes.at("moderate"); break;
        case Preset::Extended: max_nodes = preset_max_nodes.at("extended"); break;
    }

    for (ExpectedPerft pos : epd_parser()) {
        std::map<int, std::uint64_t> per_depth_values;

        for (const auto [depth, expected] : pos.depths) {
            if ((expected <= max_nodes) && (depth > 0)) {
                per_depth_values[depth] = expected;
                info.total_nodes += expected;

                if (expected >= 5000) { info.engine_nodes += expected; }
            }
        }

        if (!per_depth_values.empty()) {
            info.positions.push_back({pos.fen, pos.id, per_depth_values});
        }
    }

    return info;
}
