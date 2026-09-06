#include "presets.h"

#include <string>
#include <vector>
#include <sstream>
#include <map>
#include <cstdint>
#include <cstddef>

#include "file_manager.h"

namespace {

std::vector<ExpectedPerft> epd_parser() {
    std::vector<ExpectedPerft> pos_list;
    std::string line;

    for (const std::string& line : read_file("resources/perft_database.epd")) {
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

}

PresetInfo make_preset(Preset preset) {
    PresetInfo info;
    std::uint64_t max_nodes;
    switch (preset) {
        case Preset::Instant: max_nodes = 300000; break;
        case Preset::Fast: max_nodes = 1000000; break;
        case Preset::Moderate: max_nodes = 5000000; break;
        case Preset::Extended: max_nodes = 20000000; break;
    }

    for (ExpectedPerft pos : epd_parser()) {
        std::map<int, std::uint64_t> per_depth_values;

        for (const auto [depth, expected] : pos.depths) {
            if ((expected <= max_nodes) && (depth > 0)) {
                per_depth_values[depth] = expected;
                info.total_nodes += expected;
            }
        }

        if (!per_depth_values.empty()) {
            info.positions.push_back({pos.fen, pos.id, per_depth_values});
        }
    }

    return info;
}

std::string preset_name(Preset preset) {
    switch (preset) {
        case Preset::Instant: return "Instant";
        case Preset::Fast: return "Fast";
        case Preset::Moderate: return "Moderate";
        case Preset::Extended: return "Extended";
    }
}
