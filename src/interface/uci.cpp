#include "uci.h"

#include <variant>
#include <iostream>
#include <chrono>
#include <vector>
#include <string>
#include <optional>

#include "../notation/move_notation.h"
#include "../engine/search.h"
#include "session.h"

using namespace uci;
using namespace std::chrono;

/* --- PARSER --- */

void check_token_count(const std::vector<std::string>& tokens, std::size_t count) {
    if (tokens.size() < count) {
        throw UciError("Command is missing fields.");
    } else if (tokens.size() > count) {
        throw UciError("Command has extra fields.");
    }
}

const std::string& require_token(const std::vector<std::string>& tokens, std::size_t index) {
    if (tokens.size() > index) {
        return tokens[index];
    } else {
        throw UciError("Command is missing fields.");
    }
}

std::vector<std::string> tokenizer(const std::string_view line) {
    std::string token;
    std::vector<std::string> tokens;

    for (unsigned char c : line) {
        if (std::isspace(c)) {
            if (!token.empty()) {
                tokens.push_back(token);
                token.clear();
            }
            continue;
        }

        token += c;
    }
    if (!token.empty()) {
        tokens.push_back(token);
    }

    return tokens;
}

Command parse_position(const std::vector<std::string>& tokens) {
    std::size_t cursor = 1;

    const std::string& position_option = require_token(tokens, cursor);
    std::string fen; 
    if (position_option == "startpos") {
        fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
    } else if (position_option == "fen") {
        for (int i = 0; i < 6; i++) {
            cursor++;
            fen += require_token(tokens, cursor);
        }
    } else {
        throw UciError("Unrecognized position command.");
    }

    std::optional<std::vector<std::string>> moves;
    if (tokens.size() > cursor + 1) {
        cursor++;
        if (require_token(tokens, cursor) != "moves") {
            throw UciError("Unrecognized position command.");
        }

        moves = std::vector<std::string>{};
        while (cursor + 1 < tokens.size()) {
            cursor++;
            moves.value().push_back(require_token(tokens, cursor));
        }
        if (moves.value().empty()) {
            throw UciError("At least one move required after \"moves\".");
        }
    }

    return PositionFen{fen, moves};
}

int parse_number_string(std::string string) {
    if (string.empty()) { throw UciError("Invalid integer."); }
    if (string.length() > 9) { throw UciError("Integer too large."); }
    for (unsigned char c : string) {
        if (!std::isdigit(c)) { throw UciError("Invalid integer."); }
    }
    return std::stoi(string);
}

Command parse_go(const std::vector<std::string>& tokens) {
    const std::string& go_option = require_token(tokens, 1);
    
    if (go_option == "depth") {
        check_token_count(tokens, 3);
        int depth = parse_number_string(tokens[2]);
        if (depth == 0 || depth > 16) { throw UciError("Depth must be between 1 and 16."); }
        return GoDepth{static_cast<std::uint8_t>(depth)};
    } else if (go_option == "movetime") {
        check_token_count(tokens, 3);
        return GoMoveTime{milliseconds{parse_number_string(tokens[2])}};
    } else if (
        go_option == "wtime" ||
        go_option == "btime" ||
        go_option == "winc" ||
        go_option == "binc" ||
        go_option == "movestogo"
    ) {
        std::size_t cursor = 0;

        std::optional<milliseconds> wtime;
        std::optional<milliseconds> btime;
        std::optional<milliseconds> winc;
        std::optional<milliseconds> binc;
        std::optional<int> mtg;
        while (cursor + 1 < tokens.size()) {
            cursor++;
            const std::string& option = require_token(tokens, cursor);
            cursor++;
            const std::string& value = require_token(tokens, cursor);

            if (option == "wtime") {
                if (wtime) { throw UciError("Repeated go option."); }
                wtime = milliseconds{parse_number_string(value)};
            } else if (option == "btime") {
                if (btime) { throw UciError("Repeated go option."); }
                btime = milliseconds{parse_number_string(value)};
            } else if (option == "winc") {
                if (winc) { throw UciError("Repeated go option."); }
                winc = milliseconds{parse_number_string(value)};
            } else if (option == "binc") {
                if (binc) { throw UciError("Repeated go option."); }
                binc = milliseconds{parse_number_string(value)};
            } else if (option == "movestogo") {
                if (mtg) { throw UciError("Repeated go option."); }
                mtg = parse_number_string(value);
            } else {
                throw UciError("Unrecognized go option.");
            }
        }

        if (!wtime || !btime || (winc && !binc) || (!winc && binc)) {
            throw UciError("Malformed go command.");
        }

        return GoTimedGame{
            *wtime,
            *btime,
            winc.value_or(milliseconds(0)),
            binc.value_or(milliseconds(0)),
            mtg
        };
    } else {
        throw UciError("Unrecognized go option.");
    }
}

Command parse_uci(std::string line) {
    std::vector<std::string> tokens = tokenizer(line);
    const std::string& base_command = require_token(tokens, 0);
    if (base_command == "isready") {
        check_token_count(tokens, 1);
        return IsReady{};
    } else if (base_command == "ucinewgame") {
        check_token_count(tokens, 1);
        return NewGame{};
    } else if (base_command == "position") {
        return parse_position(tokens);
    } else if (base_command == "go") {
        return parse_go(tokens);
    } else {
        throw CliError("Unrecognized command.");
    }
}


/* --- EXECUTOR --- */

void execute_impl(const IsReady&, UciSession&) {
    std::cout << "readyok\n";
}

void execute_impl(const NewGame&, UciSession&) {}

void execute_impl(const PositionFen& cmd, UciSession& session) {
    session.set_pos(cmd.fen);
    if (cmd.moves) {
        for (const std::string& uci_move : *cmd.moves) {
            Position position = session.current_position();
            Move move = resolve_move(uci_move, position, MoveNotation::Uci);
            session.apply_move(move);
        }
    }
}


void print_bestmove(const std::optional<Move> searchedmove, const Position& position) {
    std::string bestmove = searchedmove
        ? move_notation(*searchedmove, position, MoveNotation::Uci)
        : "0000";
    std::cout << "bestmove " << bestmove << '\n';
}

void execute_impl(const GoDepth& cmd, UciSession session) {
    Position position = session.current_position();
    std::optional<Move> searchedmove = pbm_fixed_depth(position, cmd.depth).move;
    print_bestmove(searchedmove, position);
}

void execute_impl(const GoMoveTime& cmd, UciSession session) {
    Position position = session.current_position();
    std::optional<Move> searchedmove = pbm_fixed_time(position, cmd.time).move;
    print_bestmove(searchedmove, position);
}

void execute_impl(const GoTimedGame& cmd, UciSession session) {
    Position position = session.current_position();
    std::optional<Move> searchedmove = pbm_timed_game(
        position,
        position.turn() == Color::White ? cmd.wtime : cmd.btime,
        position.turn() == Color::White ? cmd.winc : cmd.binc,
        cmd.mtg
    ).move;
    print_bestmove(searchedmove, position);
}


void execute_uci(const Command& command, UciSession& session) {
    std::visit(
        [&session](const auto& concrete_command) {
            execute_impl(concrete_command, session);
        },
        command
    );
}
