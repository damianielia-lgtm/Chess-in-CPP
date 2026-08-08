#include "cli_parser.h"

#include <iostream>
#include <string>
#include <string_view>
#include <algorithm>
#include <cctype>
#include <vector>
#include <stdexcept>

#include "commands.h"

void check_token_count(const std::vector<std::string>& tokens, const std::size_t count) {
    if (tokens.size() < count) {
        throw std::invalid_argument("Command is missing fields, type 'help' to view commands.");
    } else if (tokens.size() > count) {
        throw std::invalid_argument("Command has extra fields, type 'help' to view commands.");
    }
}

const std::string& require_token(const std::vector<std::string>& tokens, const std::size_t index) {
    if (tokens.size() > index) {
        return tokens[index];
    } else {
        throw std::invalid_argument("Command is missing fields, type 'help' to view commands.");
    }
}

std::vector<std::string> tokenizer(std::string_view line) {
    std::string token;
    std::vector<std::string> tokens;
    bool in_quotes = false;

    for (char c : line) {
        if (!in_quotes && c == ' ') {
            if (!token.empty()) {
                tokens.push_back(token);
                token.clear();
            }
            continue;
        }
        
        if (c == '"') {
            in_quotes = !in_quotes;
            continue;
        }

        token += c;
    }

    if (in_quotes) { throw std::invalid_argument("Unmatched quotation."); }

    if (!token.empty()) { tokens.push_back(token); }

    return tokens;
}

Command parse_position(const std::vector<std::string>& tokens) {
    std::string position_command = require_token(tokens, 1);
    if (position_command == "show") {
        check_token_count(tokens, 2);
        return PositionShowCommand{};
    } else if (position_command == "--fen") {
        check_token_count(tokens, 3);
        return PositionFenCommand{tokens[2]};
    } else if (position_command == "--startpos") {
        check_token_count(tokens, 2);
        return PositionStartposCommand{};
    } else {
        throw std::invalid_argument("Unreconized position command.");
    }
}

Command parse_move(const std::vector<std::string>& tokens) {
    check_token_count(tokens, 2);
    return MoveCommand{tokens[1]};
}

Preset parse_preset(const std::string_view preset) {
    if (preset == "instant") {
        return Preset::Instant;
    } else if (preset == "fast") {
        return Preset::Fast;
    } else if (preset == "moderate") {
        return Preset::Moderate;
    } else if (preset == "extended") {
        return Preset::Extended;
    } else {
        throw std::invalid_argument("Invalid preset");
    }
}

Command parse_perft(const std::vector<std::string>& tokens) {
    check_token_count(tokens, 3);
    std::string perft_command = tokens[1];
    if (perft_command == "--preset") {
        return PerftPresetCommand{parse_preset(tokens[2]), PerftMode::Test};
    } else {
        throw std::invalid_argument("Unreconized perft command.");
    }
}

Command parse_benchmark(const std::vector<std::string>& tokens) {
    check_token_count(tokens, 4);
    std::string benchmark_command = tokens[1];
    if (benchmark_command == "perft") {
        std::string perft_benchmark_command = tokens[2];
        if (perft_benchmark_command == "--preset") {
            return PerftPresetCommand{parse_preset(tokens[3]), PerftMode::Benchmark};
        } else {
            throw std::invalid_argument("Unreconized benchmark perft command.");
        }
    } else {
        throw std::invalid_argument("Unreconized benchmark command.");
    }
}

int parse_depth(std::string depth_string) {
    for (unsigned char c : depth_string) {
        if (!std::isdigit(c)) { throw std::invalid_argument("Invalid depth."); }
    }
    if (depth_string == "0") { throw std::invalid_argument("Invalid depth."); }
    return std::stoi(depth_string);
}

Command parse_debug(const std::vector<std::string>& tokens) {
    check_token_count(tokens, 3);
    std::string debug_command = tokens[1];
    if (debug_command == "--depth") {
        return DebugCommand{parse_depth(tokens[2])};
    } else {
        throw std::invalid_argument("Unreconized debug command.");
    }
}

Command parse(std::string line) {
    std::vector<std::string> tokens = tokenizer(line);
    std::string base_command = require_token(tokens, 0);
    if (base_command == "position") {
        return parse_position(tokens);
    } else if (base_command == "move") {
        return parse_move(tokens);
    } else if (base_command == "perft") {
        return parse_perft(tokens);
    } else if (base_command == "benchmark") {
        return parse_benchmark(tokens);
    } else if (base_command == "debug") {
        return parse_debug(tokens);
    } else {
        throw std::invalid_argument("Unreconized command.");
    }
}