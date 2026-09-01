#include "cli_parser.h"

#include <string>
#include <string_view>
#include <cctype>
#include <vector>
#include <chrono>

#include "commands.h"
#include "../game/game.h"
#include "../errors.h"

namespace {

void check_token_count(const std::vector<std::string>& tokens, std::size_t count) {
    if (tokens.size() < count) {
        throw CommandError("Command is missing fields, type 'help' to view commands.");
    } else if (tokens.size() > count) {
        throw CommandError("Command has extra fields, type 'help' to view commands.");
    }
}

const std::string& require_token(const std::vector<std::string>& tokens, std::size_t index) {
    if (tokens.size() > index) {
        return tokens[index];
    } else {
        throw CommandError("Command is missing fields, type 'help' to view commands.");
    }
}

std::vector<std::string> tokenizer(const std::string_view line) {
    std::string token;
    std::vector<std::string> tokens;
    bool in_quotes = false;

    for (unsigned char c : line) {
        if (!in_quotes && std::isspace(c)) {
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

    if (in_quotes) { throw CommandError("Unmatched quotation."); }

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
    } else if (position_command == "--saved-fen") {
        check_token_count(tokens, 3);
        return PositionSavedFenCommand{tokens[2]};
    } else if (position_command == "--startpos") {
        check_token_count(tokens, 2);
        return PositionStartposCommand{};
    } else {
        throw CommandError("Unrecognized position command.");
    }
}

Command parse_move(const std::vector<std::string>& tokens) {
    check_token_count(tokens, 2);
    return MoveCommand{tokens[1]};
}

int parse_number_string(std::string string) {
    if (string.empty()) { throw CommandError("Invalid integer."); }
    if (string.length() > 6) { throw CommandError("Integer too large."); }
    for (unsigned char c : string) {
        if (!std::isdigit(c)) { throw CommandError("Invalid integer."); }
    }
    return std::stoi(string);
}

TimeControl parse_time(const std::string_view time) {
    auto first = time.find('+');
    auto last = time.rfind('+');
    
    if (first == std::string::npos || first != last) {
        throw CommandError("Invalid time string.");
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
        if (tokens.size() == 2) {
            return PlayCommand{};
        }

        std::string local_command = require_token(tokens, 2);

        if (local_command == "--time-control") {
            check_token_count(tokens, 4);
            std::string time_command = require_token(tokens, 3);
            return PlayCommand{parse_time(time_command)};
        } else {
            throw CommandError("Unrecognized play local command.");
        }
    } else {
        throw CommandError("Unrecognized play command.");
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
        throw CommandError("Invalid preset");
    }
}

int parse_depth(std::string depth_string) {
    int depth = parse_number_string(depth_string);
    if (depth == 0 || depth > 16) { throw CommandError("Depth must be between 1 and 16."); }
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
        throw CommandError("Unrecognized perft command.");
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
            throw CommandError("Unrecognized benchmark perft command.");
        }
    } else {
        throw CommandError("Unrecognized benchmark command.");
    }
}

Command parse_debug(const std::vector<std::string>& tokens) {
    check_token_count(tokens, 3);
    std::string debug_command = tokens[1];
    if (debug_command == "--depth") {
        return DebugCommand{parse_depth(tokens[2])};
    } else {
        throw CommandError("Unrecognized debug command.");
    }
}

Command parse_pgn(const std::vector<std::string>& tokens) {
    std::string pgn_command = require_token(tokens, 1);
    if (pgn_command == "save") {
        check_token_count(tokens, 3);
        return PgnSaveCommand{tokens[2]};
    } else if (pgn_command == "show") {
        check_token_count(tokens, 3);
        return PgnShowCommand{tokens[2]};
    } else if (pgn_command == "delete") {
        check_token_count(tokens, 3);
        return PgnDeleteCommand{tokens[2]};
    } else if (pgn_command == "list") {
        check_token_count(tokens, 2);
        return PgnListCommand{};
    } else {
        throw CommandError("Unrecognized pgn command.");
    }
}

Command parse_fen(const std::vector<std::string>& tokens) {
    std::string fen_command = require_token(tokens, 1);
    if (fen_command == "save") {
        check_token_count(tokens, 3);
        return FenSaveCommand{tokens[2]};
    } else if (fen_command == "show") {
        check_token_count(tokens, 3);
        return FenShowCommand{tokens[2]};
    } else if (fen_command == "delete") {
        check_token_count(tokens, 3);
        return FenDeleteCommand{tokens[2]};
    } else if (fen_command == "list") {
        check_token_count(tokens, 2);
        return FenListCommand{};
    } else {
        throw CommandError("Unrecognized fen command.");
    }
}

Command parse_report(const std::vector<std::string>& tokens) {
    std::string report_command = require_token(tokens, 1);
    if (report_command == "save") {
        check_token_count(tokens, 3);
        return ReportSaveCommand{tokens[2]};
    } else if (report_command == "show") {
        check_token_count(tokens, 3);
        return ReportShowCommand{tokens[2]};
    } else if (report_command == "delete") {
        check_token_count(tokens, 3);
        return ReportDeleteCommand{tokens[2]};
    } else if (report_command == "list") {
        check_token_count(tokens, 2);
        return ReportListCommand{};
    } else {
        throw CommandError("Unrecognized report command.");
    }
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
    } else if (base_command == "pgn") {
        return parse_pgn(tokens);
    } else if (base_command == "fen") {
        return parse_fen(tokens);
    } else if (base_command == "report") {
        return parse_report(tokens);
    } else if (base_command == "replay") {
        check_token_count(tokens, 2);
        return ReplayCommand{tokens[1]};
    } else if (base_command == "help") {
        check_token_count(tokens, 1);
        return HelpCommand{};
    } else {
        throw CommandError("Unrecognized command.");
    }
}
