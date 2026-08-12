#include "cli_parser.h"

#include <iostream>
#include <string>
#include <string_view>
#include <algorithm>
#include <cctype>
#include <vector>
#include <stdexcept>
#include <chrono>

#include "commands.h"
#include "../application/game.h"

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

int parse_number_string(std::string string) {
    if (string.length() > 6) { throw std::invalid_argument("Integer too large."); }
    for (unsigned char c : string) {
        if (!std::isdigit(c)) { throw std::invalid_argument("Invalid integer."); }
    }
    return std::stoi(string);
}

TimeControl parse_time(const std::string_view time) {
    auto first = time.find('+');
    auto last = time.rfind('+');
    
    if (first == std::string::npos || first != last) {
        throw std::invalid_argument("Invalid time string.");
    }
    
    std::string initial{time.substr(0, first)};
    std::string increment{time.substr(first + 1)};

    TimeControl time_control;
    time_control.initial = std::chrono::seconds{parse_number_string(initial)};
    time_control.increment = std::chrono::seconds{parse_number_string(increment)};
    
    return time_control;
}

Command parse_game(const std::vector<std::string>& tokens) {
    std::string game_command = require_token(tokens, 1);
    if (game_command == "local") {
        std::string local_command;
        try {
            local_command = require_token(tokens, 2);
        } catch (const std::invalid_argument&) {
            check_token_count(tokens, 2);
            return PlayCommand{};
        }

        if (local_command == "--time-control") {
            check_token_count(tokens, 4);
            std::string time_command = require_token(tokens, 3);
            return PlayCommand{parse_time(time_command)};
        } else {
            throw std::invalid_argument("Unreconized play local command.");
        }
    } else {
        throw std::invalid_argument("Unreconized play command.");
    }
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

int parse_depth(std::string depth_string) {
    int depth = parse_number_string(depth_string);
    if (depth == 0 || depth > 16) { throw std::invalid_argument("Depth must be between 0 and 16."); }
    return depth;
}

Command parse_perft(const std::vector<std::string>& tokens) {
    check_token_count(tokens, 3);
    std::string perft_command = tokens[1];
    if (perft_command == "--preset") {
        return PerftPresetCommand{parse_preset(tokens[2])};
    } else if (perft_command == "--depth") {
        return PerftCommand{parse_depth(tokens[2])};
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
            return BenchmarkPerftPresetCommand{parse_preset(tokens[3])};
        } else if (perft_benchmark_command == "--depth") {
            return BenchmarkPerftCommand{parse_depth(tokens[3])};
        } else {
            throw std::invalid_argument("Unreconized benchmark perft command.");
        }
    } else {
        throw std::invalid_argument("Unreconized benchmark command.");
    }
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
    } else if (base_command == "play") {
        return parse_game(tokens);
    } else if (base_command == "perft") {
        return parse_perft(tokens);
    } else if (base_command == "benchmark") {
        return parse_benchmark(tokens);
    } else if (base_command == "debug") {
        return parse_debug(tokens);
    } else if (base_command == "help") {
        return HelpCommand{};
    } else {
        throw std::invalid_argument("Unreconized command.");
    }
}
