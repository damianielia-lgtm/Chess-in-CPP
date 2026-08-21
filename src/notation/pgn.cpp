#include "pgn.h"

#include <vector>
#include <string>
#include <cassert>
#include <cmath>
#include <chrono>
#include <format>

#include "../core/move.h"
#include "../core/piece.h"
#include "../application/game.h"
#include "san.h"

std::vector<std::string> construct_pgn_lines(const GameState& game) {
    assert(game.has_ended());

    std::vector<std::string> lines;

    lines.push_back("[Event \"Chess game\"]");
    lines.push_back("[Site \"Elia.chess\"]");

    std::string date_line;
    auto date = std::format("{:%Y.%m.%d}", std::chrono::system_clock::now());
    date_line += "[Date \"";
    date_line += date;
    date_line += "\"]";
    lines.push_back(date_line);

    lines.push_back("[Round \"?\"]");
    lines.push_back("[White \"White\"]");
    lines.push_back("[Black \"Black\"]");

    std::string result;
    switch (game.result()) {
        case GameResult::BlackCheckmated:
        case GameResult::BlackResign:
        case GameResult::BlackTimeout:
            result = "1-0";
            break;

        case GameResult::WhiteCheckmated:
        case GameResult::WhiteResign:
        case GameResult::WhiteTimeout:
            result = "0-1";
            break;

        default:
            result = "1/2-1/2";
    }

    std::string result_line;
    result_line += "[Result \"";
    result_line += result;
    result_line += "\"]";
    lines.push_back(result_line);

    lines.push_back("");

    std::string moves_line;
    int move_clock = 1;
    Color turn = Color::White;
    for (
        int index = 0;
        index < game.positions().size() - 1; // last position doesn't have a move, so is excluded
        index++
    ) {
        Position position = game.positions()[index];
        Move move = game.moves()[index];

        if (turn == Color::White) {
            moves_line += std::to_string(move_clock);
            moves_line += ". ";
            move_clock++;
        }

        moves_line += to_san(position, move);
        moves_line += ' ';

        turn = (turn == Color::White)
            ? Color::Black
            : Color::White;

        if (moves_line.size() >= 75) {
            lines.push_back(moves_line);
            moves_line.clear();
        }
    }
    moves_line += result;
    lines.push_back(moves_line);

    return lines;
}
