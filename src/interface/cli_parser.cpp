#include "cli.h"

#include <string>
#include <string_view>
#include <cctype>
#include <vector>
#include <chrono>
#include <cstddef>
#include <cstdint>

#include "../game/game.h"
#include "../errors.h"

using namespace std::chrono;

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
    const std::string& position_command = require_token(tokens, 1);
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
    time_control.initial = seconds{parse_number_string(initial)};
    time_control.increment = seconds{parse_number_string(increment)};
    
    return time_control;
}

Command parse_game(const std::vector<std::string>& tokens) {
    const std::string& game_command = require_token(tokens, 1);
    if (game_command == "local") {
        const std::string& local_command = require_token(tokens, 2);
        if (local_command == "--time-control") {
            check_token_count(tokens, 4);
            return PlayLocalCommand{parse_time(tokens[3])};
        } if (local_command == "--untimed") {
            check_token_count(tokens, 3);
            return PlayLocalCommand{};
        } else {
            throw CommandError("Unrecognized play local command.");
        }
    } else if (game_command == "engine") {
        if (require_token(tokens, 2) != "--player-color") {
            throw CommandError("Unrecognized play engine command.");
        }
        const std::string& player_color_field = require_token(tokens, 3);
        Color player_color;
        if (player_color_field == "white") {
            player_color = Color::White;
        } else if (player_color_field == "black") {
            player_color = Color::Black;
        } else {
            throw CommandError("Invalid player color.");
        }

        const std::string& time_control_field = require_token(tokens, 4);
        if (time_control_field == "--time-control") {
            check_token_count(tokens, 6);
            return PlayEngineCommand{parse_time(tokens[5]), player_color};
        } else if (time_control_field == "--untimed") {
            check_token_count(tokens, 5);
            return PlayEngineCommand{std::nullopt, player_color};
        } else {
            throw CommandError("Unrecognized play local command.");
        }
    } else {
        throw CommandError("Unrecognized play command.");
    }
}

Preset parse_preset(const std::string_view preset) {
    if (preset == "fast") {
        return Preset::Fast;
    } else if (preset == "moderate") {
        return Preset::Moderate;
    } else if (preset == "extended") {
        return Preset::Extended;
    } else {
        throw CommandError("Invalid preset");
    }
}

std::uint8_t parse_depth(std::string depth_string) {
    int depth = parse_number_string(depth_string);
    if (depth == 0 || depth > 16) { throw CommandError("Depth must be between 1 and 16."); }
    return static_cast<std::uint8_t>(depth);
}

Command parse_perft(const std::vector<std::string>& tokens) {
    check_token_count(tokens, 4);
    const std::string& perft_command = tokens[1];
    if (perft_command == "test") {
        const std::string& test_mode = tokens[2];
        if (test_mode == "--preset") {
            return PerftTestPresetCommand{parse_preset(tokens[3])};
        } else if (test_mode == "--depth") {
            return PerftTestCommand{parse_depth(tokens[3])};
        } else {
            throw CommandError("Unrecognized perft test command.");
        }
    } else if (perft_command == "benchmark") {
        const std::string& benchmark_mode = tokens[2];
        if (benchmark_mode == "--preset") {
            return PerftBenchmarkPresetCommand{parse_preset(tokens[3])};
        } else if (benchmark_mode == "--depth") {
            return PerftBenchmarkCommand{parse_depth(tokens[3])};
        } else {
            throw CommandError("Unrecognized perft benchmark command.");
        }
    } else {
        throw CommandError("Unrecognized perft command.");
    }
}

Command parse_debug(const std::vector<std::string>& tokens) {
    check_token_count(tokens, 3);
    const std::string& debug_command = tokens[1];
    if (debug_command == "--depth") {
        return DebugCommand{parse_depth(tokens[2])};
    } else {
        throw CommandError("Unrecognized debug command.");
    }
}

Command parse_pgn(const std::vector<std::string>& tokens) {
    const std::string& pgn_command = require_token(tokens, 1);
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
    } else if (pgn_command == "import") {
        check_token_count(tokens, 3);
        return PgnImportCommand{tokens[2]};
    } else if (pgn_command == "export") {
        check_token_count(tokens, 4);
        return PgnExportCommand{tokens[2], tokens[3]};
    } else {
        throw CommandError("Unrecognized pgn command.");
    }
}

Command parse_fen(const std::vector<std::string>& tokens) {
    const std::string& fen_command = require_token(tokens, 1);
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
    } else if (fen_command == "import") {
        check_token_count(tokens, 3);
        return FenImportCommand{tokens[2]};
    } else if (fen_command == "export") {
        check_token_count(tokens, 4);
        return FenExportCommand{tokens[2], tokens[3]};
    } else {
        throw CommandError("Unrecognized fen command.");
    }
}

Command parse_report(const std::vector<std::string>& tokens) {
    const std::string& report_command = require_token(tokens, 1);
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

Command parse_config(const std::vector<std::string>& tokens) {
    const std::string& config_command = require_token(tokens, 1);
    if (config_command == "show") {
        check_token_count(tokens, 2);
        return ConfigShowCommand{};
    } else if (config_command == "set") {
        const std::string& set_field = require_token(tokens, 2);
        const std::string& set_value = require_token(tokens, 3);
        check_token_count(tokens, 4);
        if (set_field == "player-1") {
            return ConfigSetPlayer1Command{set_value};
        } else if (set_field == "player-2") {
            return ConfigSetPlayer2Command{set_value};
        } else if (set_field == "event") {
            return ConfigSetEventCommand{set_value};
        } else if (set_field == "site") {
            return ConfigSetSiteCommand{set_value};
        } else if (set_field == "export-clocks") {
            if (set_value == "true") {
                return ConfigSetExportClocksCommand{true};
            } else if (set_value == "false") {
                return ConfigSetExportClocksCommand{false};
            } else {
                throw CommandError("Unrecognized export-clocks value.");
            }
        } else if (set_field == "move-notation") {
            if (set_value == "uci") {
                return ConfigSetMoveNotationCommand{MoveNotation::Uci};
            } else if (set_value == "san") {
                return ConfigSetMoveNotationCommand{MoveNotation::San};
            } else {
                throw CommandError("Unrecognized move-input value.");
            }
        } else if (set_field == "board-orientation") {
            if (set_value == "white") {
                return ConfigSetBoardOrientationCommand{BoardOrientation::White};
            } else if (set_value == "black") {
                return ConfigSetBoardOrientationCommand{BoardOrientation::Black};
            } else {
                throw CommandError("Unrecognized board-orientation value.");
            }
        } else if (set_field == "track-benchmark-stats") {
            if (set_value == "true") {
                return ConfigSetBenchmarkStatsTrackingCommand{true};
            } else if (set_value == "false") {
                return ConfigSetBenchmarkStatsTrackingCommand{false};
            } else {
                throw CommandError("Unrecognized track-stats value.");
            }
        } else {
            throw CommandError("Unrecognized set field.");
        }
    } else {
        throw CommandError("Unrecognized config command.");
    }
}

Command parse_engine(const std::vector<std::string>& tokens) {
    const std::string& engine_command = require_token(tokens, 1);
    if (engine_command == "static-eval") {
        check_token_count(tokens, 2);
        return EngineStaticEvalCommand{};
    } else if (engine_command == "dynamic-eval") {
        check_token_count(tokens, 4);
        const std::string& search_limit = tokens[2];
        if (search_limit == "--time-ms") {
            return EngineDynamicEvalTimeCommand{milliseconds{parse_number_string(tokens[3])}};
        } else if (search_limit == "--depth") {
            return EngineDynamicEvalDepthCommand{parse_depth(tokens[3])};
        } else {
            throw CommandError("Unrecognized engine dynamic-eval command.");
        }
    } else if (engine_command == "bestmove") {
        check_token_count(tokens, 4);
        const std::string& search_limit = tokens[2];
        if (search_limit == "--time-ms") {
            return EngineBestmoveTimeCommand{milliseconds{parse_number_string(tokens[3])}};
        } else if (search_limit == "--depth") {
            return EngineBestmoveDepthCommand{parse_depth(tokens[3])};
        } else {
            throw CommandError("Unrecognized engine bestmove command.");
        }
    } else if (engine_command == "rank") {
        check_token_count(tokens, 4);
        if (tokens[2] == "--depth") {
            return EngineRankMovesCommand{parse_depth(tokens[3])};
        } else {
            throw CommandError("Unrecognized engine bestmove command.");
        }
    } else if (engine_command == "benchmark") {
        check_token_count(tokens, 4);
        const std::string& benchmark_setting = tokens[2];
        if (benchmark_setting == "--preset") {
            return EngineBenchmarkPresetCommand{parse_preset(tokens[3])};
        } else if (benchmark_setting == "--depth") {
            return EngineBenchmarkCommand{parse_depth(tokens[3])};
        } else {
            throw CommandError("Unrecognized engine benchmark command.");
        }
    } else if (engine_command == "profile") {
        check_token_count(tokens, 5);
        const std::string& profile_mode = tokens[2];
        if (profile_mode == "pruning") {
            const std::string& profile_setting = tokens[3];
            if (profile_setting == "--preset") {
                return EngineProfilePruningPresetCommand{parse_preset(tokens[4])};
            } else if (profile_setting == "--depth") {
                return EngineProfilePruningCommand{parse_depth(tokens[4])};
            } else {
                throw CommandError("Unrecognized engine profile pruning command.");
            }
        } else {
            throw CommandError("Unrecognized engine profile command.");
        }  
    } else {
        throw CommandError("Unrecognized engine command.");
    }
}

}

Command parse(std::string line) {
    std::vector<std::string> tokens = tokenizer(line);
    const std::string& base_command = require_token(tokens, 0);
    if (base_command == "position") {
        return parse_position(tokens);
    } else if (base_command == "move") {
        return parse_move(tokens);
    } else if (base_command == "play") {
        return parse_game(tokens);
    } else if (base_command == "perft") {
        return parse_perft(tokens);
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
    } else if (base_command == "analyze") {
        check_token_count(tokens, 1);
        return AnalyzeCommand{};
    } else if (base_command == "help") {
        check_token_count(tokens, 1);
        return HelpCommand{};
    }  else if (base_command == "config") {
        return parse_config(tokens);
    } else if (base_command == "engine") {
        return parse_engine(tokens);
    } else {
        throw CommandError("Unrecognized command.");
    }
}
