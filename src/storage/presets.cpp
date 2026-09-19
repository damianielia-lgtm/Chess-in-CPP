#include "presets.h"

#include <string>
#include <string_view>
#include <vector>
#include <sstream>
#include <map>
#include <cstdint>
#include <cstddef>

#include "../core/position.h"
#include "../errors.h"
#include "file_manager.h"

namespace {

static constexpr int MAX_NODES = 20000000;
static constexpr int MIN_NODES = 5000;

std::string trim(const std::string& str) {
    const auto start = str.find_first_not_of(" \t\n\r");
    const auto end = str.find_last_not_of(" \t\n\r");
    
    if (start == std::string::npos) { return ""; }
    
    return str.substr(start, end - start + 1);
}

void parse_epd_line(const std::string& line, PresetData& data) {
    if (line.empty()) { return; }
    if (line.size() < 2) { throw EpdError("Malformed line."); }

    Position position;
    std::string id;

    std::vector<std::string> tokens;
    std::stringstream ss(line);
    std::string token;
    while (std::getline(ss, token, ';')) {
        token = trim(token);
        if (!token.empty()) {
            tokens.push_back(token);
        }
    }

    try {
        position = Position(tokens[0]);
        tokens.pop_back();
    } catch (const FenError&) {
        throw EpdError("Invalid fen \"" + position.to_fen() + '\"');
    }

    if (!tokens[1].starts_with("id ")) {
        id = "Position " + std::to_string(data.positions_count + 1);
    } else {
        const std::string& id_field = tokens[1];

        std::string quoted_id = id_field.substr(3);

        if (
            quoted_id.size() < 3 ||
            quoted_id.front() != '"' ||
            quoted_id.back() != '"'
        ) {
            throw EpdError("Malformed ID.");
        }

        id = quoted_id.substr(1, quoted_id.size() - 2);
        tokens.pop_back();
    }

    bool consumed_position = false;
    
    for (const std::string& field : tokens) {        
        if (!field.starts_with("D")) {
            continue;
        }

        std::string depth_field = field.substr(1, field.find(' ') - 1);
        if (depth_field.size() > 2 || depth_field.empty()) {
            throw EpdError("Invalid depth " + depth_field);
        }
        for (unsigned char c : depth_field) {
            if (!std::isdigit(c)) { throw EpdError("Invalid depth " + depth_field); }
        }

        std::string expected_field = field.substr(field.find(' ') + 1);
        if (expected_field.size() > 20 || expected_field.empty()) {
            throw EpdError("Invalid node count " + expected_field);
        }
        for (unsigned char c : expected_field) {
            if (!std::isdigit(c)) { throw EpdError("Invalid node count " + expected_field); }
        }

        std::uint8_t depth = std::stoi(depth_field);
        std::uint64_t expected = std::stoull(expected_field);

        if (
            expected <= MAX_NODES &&
            expected >= MIN_NODES &&
            depth > 0
        ) {
            data.total_nodes += expected;
            data.suites.push_back({position, id, depth, expected});
            consumed_position = true;
        }
    }

    if (consumed_position) { data.positions_count++; }
}

}

PresetData make_preset(Preset preset) {
    PresetData data;

    std::string suite_path;
    switch (preset) {
        case Preset::Fast: suite_path = "resources/fast-suite.epd"; break;
        case Preset::Moderate: suite_path = "resources/moderate-suite.epd"; break;
        case Preset::Extended: suite_path = "resources/extended-suite.epd"; break;
    }

    for (const std::string& line : read_file(suite_path)) {
        parse_epd_line(line, data);
    }

    return data;
}

std::string preset_name(Preset preset) {
    switch (preset) {
        case Preset::Fast: return "Fast";
        case Preset::Moderate: return "Moderate";
        case Preset::Extended: return "Extended";
    }
}
