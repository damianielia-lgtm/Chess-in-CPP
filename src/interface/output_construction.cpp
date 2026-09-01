#include "output_construction.h"

#include <string>
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

std::string format_time(std::chrono::milliseconds duration) {
    using namespace std::chrono;

    auto hms = hh_mm_ss{duration};
    
    std::string result = std::format("{}:{:02}:{:02}.{}", 
        hms.hours().count(), 
        hms.minutes().count(), 
        hms.seconds().count(), 
        hms.subseconds().count() / 100 // adjust for 1 digit millisecond/fraction
    );

    return result;
}

std::vector<std::string> construct_board_lines(
    const Position& position,
    bool flip,
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
    
    lines.push_back((flip)
        ? "    h   g   f   e   d   c   b   a    "
        : "    a   b   c   d   e   f   g   h    ");

    return lines;
}

namespace {

std::string construct_material_line(const GameSnapshot& game, const GameMetadata& metadata) {
    std::string material_line;
    int material_comparison = game.material_comparison();

    material_line += metadata.white_name + ": ";
    for (bool first = true; const Piece white_captured_piece : game.captures(Color::White)) {
        if (!first) { material_line += ", "; }
        material_line += white_captured_piece.symbol();
        first = false;
    }
    if (material_comparison > 0) {
        material_line += " + " + std::to_string(material_comparison);
    }

    material_line += " | ";
    
    material_line += metadata.black_name + ": ";
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
    }
}

}

std::vector<std::string> construct_game_lines(
    const GameSnapshot& snapshot,
    std::optional<std::string> error,
    std::optional<GameResult> result_message,
    const GameMetadata& metadata
) {
    std::vector<std::string> lines;

    for (const std::string& line : construct_board_lines(snapshot.position(), false, snapshot.last_move())) {
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
