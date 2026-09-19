#include "display.h"

#include <iostream>
#include <string>
#include <string_view>
#include <chrono>
#include <format>
#include <vector>
#include <optional>
#include <cassert>

#include "../core/position.h"
#include "../core/square.h"
#include "../core/piece.h"
#include "../game/game.h"
#include "../movegen/attacks.h"
#include "../config.h"

using namespace std::chrono;

void print_lines(const std::vector<std::string>& lines) {
    for (const std::string& line : lines) {
        std::cout << line << '\n';
    }
}

namespace {

constexpr std::string_view help_content =
    "Core\n"
    "   config show                                                                   show current configuration\n"
    "   config set <field> <value>                                                    set configurations\n"
    "   position show                                                                 print current position\n"
    "   position --startpos                                                           set starting position\n"
    "   position --fen \"<fen string>\"                                                 load in specific fen\n"
    "   position --saved-fen <saved fen name>                                         load in saved fen\n"
    "   move <move>                                                                   apply move to current position\n\n"

    "Game features\n"
    "   play local --untimed                                                          start a local untimed game\n"
    "   play local --time-control <initial+increment>                                 start a local timed game\n"
    "   play online                                                                   start an online game\n"
    "   play engine --player-color {white|black}                                      play against the engine\n"
    "   replay <saved pgn name>                                                       replay a saved game\n"
    "   analyze                                                                       analyze current position\n\n"

    "Engine\n"
    "   engine static-eval                                                            print static evaluation of current position\n"
    "   engine dynamic-eval                                                           print minimax evaluation of current position\n"
    "   engine bestmove                                                               print best move for current position\n"
    "   engine rank                                                                   print all legal moves ranked by evaluation\n\n"

    "PGN management\n"
    "   pgn list                                                                      list saved PGNs\n"
    "   pgn save <name>                                                               save last game as PGN\n"
    "   pgn show <name>                                                               print a saved PGN\n"
    "   pgn delete <name>                                                             delete a saved PGN\n"
    "   pgn import <file>                                                             load a pgn to game directory\n"
    "   pgn export <name> <directory>                                                 export a saved pgn from game directory\n\n"

    "FEN management\n"
    "   fen list                                                                      list saved FENs\n"
    "   fen save <name>                                                               save current position FEN string under a name\n"
    "   fen show <name>                                                               print a saved FEN\n"
    "   fen delete <name>                                                             delete a saved FEN\n"
    "   fen import <file>                                                             load a fen to game directory\n"
    "   fen export <name> <directory>                                                 export a saved fen from game directory\n\n"

    "Report management\n"
    "   report list                                                                   list saved reports\n"
    "   report save <name>                                                            save latest report under a name\n"
    "   report show <name>                                                            print a saved reports\n"
    "   report delete <name>                                                          delete a saved report\n\n"

    "Dev Commands\n"
    "   perft test --preset <fast|moderate|extended>                                  test engine corectness through the database\n"
    "   perft test --depth <n>                                                        test corectness on a current position.\n"
    "   perft benchmark --preset <fast|moderate|extended>                             test movegen speed through the database\n"
    "   perft benchmark --depth <n>                                                   test movegen speed on current position\n"
    "   engine benchmark --preset <fast|moderate|extended>                            test engine speed through the database\n"
    "   engine benchmark --depth <n>                                                  test engine speed on current position\n"
    "   debug --depth <n>                                                             recusively go through a position and compare with stockfish.\n";
}

std::vector<std::string> help_lines() { return {std::string(help_content)}; }

std::vector<std::string> construct_board_lines(
    const Position& position,
    BoardOrientation orientation,
    std::optional<Move> move
) {
    std::vector<std::string> lines;

    Square king_in_check =
        is_attacked_square(
            position,
            position.king_square(position.turn()),
            position.opposite_turn()
        )
        ? position.king_square(position.turn())
        : Square(); // invalid index

    Square to_highlight1 = Square(); // invalid index
    Square to_highlight2 = Square(); // invalid index
    if (move) {
        to_highlight1 = move.value().origin();
        to_highlight2 = move.value().target();
    }

    lines.push_back((orientation == BoardOrientation::White)
        ? "    a   b   c   d   e   f   g   h    "
        : "    h   g   f   e   d   c   b   a    ");

    lines.push_back("  +---+---+---+---+---+---+---+---+  ");

    for (int rank = 7; rank >= 0; --rank) {
        int display_rank = orientation == BoardOrientation::White ? rank : 7 - rank;
        std::string board_line;

        board_line += std::to_string(display_rank + 1);
        board_line += " | ";

        for (int file = 0; file <= 7; file++) {
            int display_file = orientation == BoardOrientation::White ? file : 7 - file;
            Square square(display_rank * 8 + display_file);

            if (square == king_in_check) {
                board_line += "\033[31m";
                board_line += position.piece_at(square).symbol();
                board_line += "\033[0m";
            } else if (square == to_highlight1 || square == to_highlight2) {
                board_line += "\x1b[33m";
                board_line += position.piece_at(square).symbol();
                board_line += "\033[0m";
            } else {
                board_line += position.piece_at(square).symbol();
            }

            board_line += " | ";
        }

        board_line += std::to_string(display_rank + 1);
        lines.push_back(board_line);
        lines.push_back("  +---+---+---+---+---+---+---+---+  ");
    }
    
    lines.push_back((orientation == BoardOrientation::White)
        ? "    a   b   c   d   e   f   g   h    "
        : "    h   g   f   e   d   c   b   a    ");

    return lines;
}

namespace {

std::string format_time(milliseconds duration) {
    auto hms = hh_mm_ss{duration};
    
    std::string result = std::format("{}:{:02}:{:02}.{}", 
        hms.hours().count(), 
        hms.minutes().count(), 
        hms.seconds().count(), 
        hms.subseconds().count() / 100 // adjust for 1 digit millisecond/fraction
    );

    return result;
}

std::string construct_material_line(const GameSnapshot& game, const GameMetadata& metadata) {
    std::string material_line;
    int material_comparison = game.material_comparison();

    material_line += metadata.white_name + ':';
    if (!game.captures(Color::White).empty()) { material_line += ' '; }
    for (bool first = true; const Piece white_captured_piece : game.captures(Color::White)) {
        if (!first) { material_line += ", "; }
        material_line += white_captured_piece.symbol();
        first = false;
    }
    if (material_comparison > 0) {
        material_line += " + " + std::to_string(material_comparison);
    }

    material_line += " | ";
    
    material_line += metadata.black_name + ':';
    if (!game.captures(Color::Black).empty()) { material_line += ' '; }
    for (bool first = true; const Piece black_captured_piece : game.captures(Color::Black)) {
        if (!first) { material_line += ", "; }
        material_line += black_captured_piece.symbol();
        first = false;
    }
    if (material_comparison < 0) {
        material_line += " + " + std::to_string(material_comparison * -1);
    }

    return material_line;
}

std::string construct_clock_line(const GameSnapshot& game, const GameMetadata& metadata) {
    std::string clock_line;
    clock_line += metadata.white_name + ": " + format_time(game.clock(Color::White));
    clock_line += " | ";
    clock_line += metadata.black_name + ": " + format_time(game.clock(Color::Black));

    return clock_line;
}

std::string construct_prompt_line(const GameSnapshot& game, const GameMetadata& metadata) {
    return game.turn() == Color::White
        ? metadata.white_name + " to move. "
        : metadata.black_name + " to move. ";
}

std::string construct_game_end_message(const GameResult result, const GameMetadata& metadata) {
    switch (result) {
        case GameResult::White_by_Checkmate:
            return metadata.white_name + " has won by checkmate.";
        case GameResult::Black_by_Checkmate:
            return metadata.black_name + " has won by checkmate.";
        case GameResult::White_by_Resignation:
            return metadata.black_name + " has resigned. " + metadata.white_name + " wins.";
        case GameResult::Black_by_Resignation:
            return metadata.white_name + " has resigned. " + metadata.black_name + " wins.";
        case GameResult::White_by_Timeout:
            return metadata.black_name + " has timed-out. " + metadata.white_name + " wins.";
        case GameResult::Black_by_Timeout:
            return metadata.white_name + " has timed-out. " + metadata.black_name + " wins.";
        case GameResult::White_by_Unknown:
            return metadata.white_name + " has won.";
        case GameResult::Black_by_Unknown:
            return metadata.black_name + " has won.";

        case GameResult::Draw_by_Stalemate:
            return "The game has ended in a stalemate.";
        case GameResult::Draw_by_InsufficientMaterial:
            return "The game has ended in a draw due to insufficient material.";
        case GameResult::Draw_by_FiftyMove:
            return "The game has ended in a draw due to the fifty move rule.";
        case GameResult::Draw_by_ThreefoldRepetition:
            return "The game has ended in a draw due to threefold repetition.";
        case GameResult::Draw_by_Agreement:
            return "The game has ended in a draw by agreement.";
        case GameResult::Draw_by_Unknown:
            return "The game has ended in a draw.";

        case GameResult::Unknown_End:
            return "The game has ended.";
    }
}

std::vector<std::string> construct_game_lines(
    const GameSnapshot& snapshot,
    std::optional<std::string> error,
    std::optional<GameResult> result_message,
    const GameMetadata& metadata,
    BoardOrientation board_orientation
) {
    std::vector<std::string> lines;

    for (
        const std::string& line :
        construct_board_lines(snapshot.position(), board_orientation, snapshot.last_move())
    ) {
        lines.push_back(line);
    }

    if (snapshot.has_clock_data()) { lines.push_back(construct_clock_line(snapshot, metadata)); }

    lines.push_back(construct_material_line(snapshot, metadata));

    if (error) {
        std::string error_line;
        error_line += "\033[31m";
        error_line += error.value();
        error_line += "\033[0m";
        lines.push_back(error_line);
    }

    if (result_message) {
        lines.push_back("\033[34m" + construct_game_end_message(*result_message, metadata) + "\033[0m");
        lines.push_back("");
    } else {
        lines.push_back(construct_prompt_line(snapshot, metadata));
    }

    return lines;
}

}

ProgressDisplay::ProgressDisplay(std::uint64_t total_nodes)
    : total_nodes_(total_nodes)
{
    print_progress(0);
}

ProgressDisplay::~ProgressDisplay() {
    std::cerr << "\r\033[2K" << std::flush;
}

void ProgressDisplay::advance(std::uint64_t nodes) {
    completed_nodes_ += nodes;
    print_progress(completed_nodes_);
}

void ProgressDisplay::print_progress(std::uint64_t completed) {
    int percentage = static_cast<int>(completed * 100 / total_nodes_);
    int bar_len = percentage / 5;

    std::cerr << "\r[";
    for (int i = 0; i < 20; i++) {
        if (i < bar_len) { std::cerr << "="; }
        else if (i == bar_len) { std::cerr << ">"; }
        else { std::cerr << " "; }
    }
    std::cerr << "] " << percentage << "%" << std::flush;
}

namespace {

void clear_lines(std::size_t lines_number) {
    for (std::size_t i = 0; i < lines_number; i++) { // Clear line by line, going up
        std::cout << "\r"; // Go to the start of line
        std::cout << "\033[2K"; // Clear line
        if (i + 1 < lines_number) { std::cout << "\033[A"; } // Go up if another line remains
    }
}

}

void GameDisplay::update(const GameSnapshot& game, BoardOrientation board_orientation) {
    clear_lines(rendered_line_count_);

    std::vector<std::string> lines = construct_game_lines(
        game,
        error_message_,
        result_,
        metatdata_,
        board_orientation
    );
    rendered_line_count_ = lines.size();

    std::string prompt_line = lines.back();
    lines.pop_back();
    print_lines(lines);
    std::cout << prompt_line;
}

void GameDisplay::clear_rendered_area() {
    clear_lines(rendered_line_count_);
    rendered_line_count_ = 0;
}
