#include "pgn.h"

#include <vector>
#include <string>
#include <cassert>
#include <chrono>
#include <format>
#include <optional>

#include "../core/move.h"
#include "../core/piece.h"
#include "../interface/output_construction.h"
#include "san.h"

using namespace std::chrono;

std::vector<std::string> construct_pgn_lines(const Game& game) {
    assert(game.has_ended());

    std::vector<std::string> lines;

    lines.push_back("[Event \"Chess game\"]");
    lines.push_back("[Site \"Elia.chess\"]");

    auto date = std::format("{:%Y.%m.%d}", system_clock::now());
    lines.push_back("[Date \"" + date + "\"]");

    lines.push_back("[Round \"?\"]");

    lines.push_back("[White \"" + game.name(Color::White) + "\"]");
    lines.push_back("[Black \"" + game.name(Color::Black) + "\"]");

    std::string result;
    switch (game.result()) {
        case GameResult::White_by_Checkmate:
        case GameResult::White_by_Resignation:
        case GameResult::White_by_Timeout:
        case GameResult::White_by_Unknown:
            result = "1-0";
            break;
        case GameResult::Black_by_Checkmate:
        case GameResult::Black_by_Resignation:
        case GameResult::Black_by_Timeout:
        case GameResult::Black_by_Unknown:
            result = "0-1";
            break;
        default:
            result = "1/2-1/2";
    }

    lines.push_back("[Result \"" + result + "\"]");

    std::string timecontrol;
    if (game.is_timed_game()) {
        std::string initial = std::to_string(
            duration_cast<seconds>(game.metadata().time_control->initial).count()
        );
        std::string increment = std::to_string(
            duration_cast<seconds>(game.metadata().time_control->increment).count()
        );
        
        timecontrol = (increment != "0")
            ? initial + '+' + increment
            : initial;
    } else {
        timecontrol = '-';
    }
    lines.push_back("[TimeControl \"" + timecontrol + "\"]");

    lines.push_back("");

    std::string moves_line;
    int move_clock = 1;
    bool added_comment = false;

    for (std::size_t index = 1; index < game.all_snapshots().size(); index++) {
        Position position = game.all_snapshots()[index - 1].position();
        std::optional<Move> move = game.all_snapshots()[index].last_move();

        if (!move) { continue; }

        if (position.turn() == Color::White) {
            moves_line += std::to_string(move_clock);
            moves_line += ". ";
            move_clock++;
        } else if (added_comment) {
            moves_line += std::to_string(move_clock - 1);
            moves_line += "... ";
        }

        moves_line += to_san(position, *move) + ' ';

        if (game.is_timed_game()) {
            milliseconds clock = game.all_snapshots()[index].clock(position.turn());
            moves_line += "{[%clk " + format_time(clock) + "]} ";
            added_comment = true;
        } else {
            added_comment = false;
        }

        if (moves_line.size() >= 75) {
            lines.push_back(moves_line);
            moves_line.clear();
        }
    }
    moves_line += result;
    lines.push_back(moves_line);

    return lines;
}
