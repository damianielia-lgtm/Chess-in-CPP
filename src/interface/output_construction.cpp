#include "output_construction.h"

#include <string>
#include <chrono>
#include <format>
#include <vector>
#include <optional>

#include "../core/position.h"
#include "../core/square.h"
#include "../core/piece.h"
#include "../application/game.h"
#include "../movegen/attacks.h"

std::string format_time(std::chrono::milliseconds duration) {
    using namespace std::chrono;

    auto mins = duration_cast<minutes>(duration);
    duration -= mins;
    auto secs = duration_cast<seconds>(duration);
    duration -= secs;
    auto ms = duration_cast<milliseconds>(duration);
    return std::format("{:02}:{:02}.{:03}", mins.count(), secs.count(), ms.count());
}

std::vector<std::string> construct_board_lines(const Position& position, bool flip) {
    std::vector<std::string> lines;

    Square king_in_check =
        is_attacked_square(
            position,
            position.king_square(position.turn()),
            position.opposite_turn()
        )
        ? position.king_square(position.turn())
        : Square(); // invalid index

    lines.push_back((flip)
        ? "    h   g   f   e   d   c   b   a    "
        : "    a   b   c   d   e   f   g   h    ");

    lines.push_back("  +---+---+---+---+---+---+---+---+  ");

    for (int rank = 7; rank >= 0; --rank) {
        int display_rank = flip ? 7 - rank : rank;
        std::string board_line;

        board_line += std::to_string(display_rank + 1);
        board_line += " | ";

        for (int file = 0; file <= 7; file++) {
            int display_file = flip ? 7 - file : file;
            Square square(display_rank * 8 + display_file);

            if (square == king_in_check) {
                board_line += "\033[31m";
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
    
    lines.push_back((flip)
        ? "    h   g   f   e   d   c   b   a    "
        : "    a   b   c   d   e   f   g   h    ");

    return lines;
}

std::string construct_material_line(const GameState& game) {
    std::string material_line;
    int material_comparision = game.material_comparison();

    material_line += "White: ";
    for (bool first = true; const Piece white_captured_piece : game.captures(Color::White)) {
        if (!first) { material_line += ", "; }
        material_line += white_captured_piece.symbol();
        first = false;
    }
    if (material_comparision > 0) {
        material_line += " + " + std::to_string(material_comparision);
    }

    material_line += " | ";
    
    material_line += "Black: ";
    for (bool first = true; const Piece black_captured_piece : game.captures(Color::Black)) {
        if (!first) { material_line += ", "; }
        material_line += black_captured_piece.symbol();
        first = false;
    }
    if (material_comparision < 0) {
        material_line += " + " + std::to_string(material_comparision * -1);
    }

    return material_line;
}

std::string construct_clock_line(const GameState& game) {
    std::string clock_line;
    clock_line += "White: " + format_time(game.current_clock(Color::White));
    clock_line += " | ";
    clock_line += "Black: " + format_time(game.current_clock(Color::Black));

    return clock_line;
}

std::string construct_prompt_line(const GameState& game) {
    return std::string(game.turn_name()) + " to move. ";
}

std::string construct_game_end_message(const GameState& game) {
    assert(game.has_ended());

    switch (game.result()) {
        case GameResult::WhiteCheckmate:
            return "White has won by checkmate.";
        case GameResult::BlackCheckmate:
            return "Black has won by checkmate.";
        case GameResult::WhiteResign:
            return "White has resigned. Black wins";
        case GameResult::BlackResign:
            return "Black has resigned. White wins";
        case GameResult::WhiteTimeout:
            return "White has timed-out. Black wins";
        case GameResult::BlackTimeout:
            return "Black has timed-out. White wins";

        case GameResult::Stalemate:
            return "The game has ended in a stalemate.";
        case GameResult::InsufficientMaterial:
            return "The game has ended in a draw due to insufficient material.";
        case GameResult::FiftyMoveRule:
            return "The game has ended in a draw due to the fifty move rule.";
        case GameResult::ThreefoldRepetition:
            return "The game has ended in a draw due to threefold repetition.";
        case GameResult::Agreement:
            return "The game has ended in a draw by agreement";
    }
}

std::vector<std::string> construct_game_lines(
    const GameState& game,
    bool show_clock,
    std::optional<std::string> error
) {
    std::vector<std::string> lines;

    for (const std::string& line : construct_board_lines(game.current_position(), false)) {
        lines.push_back(line);
    }

    if (show_clock) { lines.push_back(construct_clock_line(game)); }

    lines.push_back(construct_material_line(game));

    if (error) {
        std::string error_line;
        error_line += "\033[31m";
        error_line += error.value();
        error_line += "\033[0m";
        lines.push_back(error_line);
    }

    if (game.has_ended()) {
        std::string end_message;
        end_message += "\033[34m";
        end_message += construct_game_end_message(game);
        end_message += "\033[0m\n";
        lines.push_back(end_message);
    } else {
        lines.push_back(construct_prompt_line(game));
    }

    return lines;
}
