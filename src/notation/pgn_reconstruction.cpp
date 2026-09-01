#include "pgn.h"

#include <string>
#include <optional>
#include <chrono>

#include "../game/game.h"
#include "../errors.h"
#include "san.h"

using namespace std::chrono;

Game reconstruct_game(const ParsedPGN& pgn_data) {
    bool keep_clocks = true;
    if (!pgn_data.time_control) { keep_clocks = false; }
    if (pgn_data.plies.empty()) { keep_clocks = false; }
    for (const ParsedPly& ply : pgn_data.plies) {
        if (!ply.clock_after) { keep_clocks = false; }
    }

    Game game(
        pgn_data.white_name,
        pgn_data.black_name,
        keep_clocks ? pgn_data.time_control : std::nullopt,
        pgn_data.starting_position
    );

    int movenumber = 1;
    for (const ParsedPly& ply : pgn_data.plies) {
        if (keep_clocks) {
            milliseconds time_elpsed =
                game.live_snapshot().clock(game.live_position().turn())
                - *ply.clock_after
                + pgn_data.time_control->increment;

            game.consume_time(time_elpsed);
        }

        std::string san_move = ply.san_move;

        try {
            Move move = resolve_san(game.live_position(), san_move);
            game.play_move(move);
        } catch (const IllegalMoveError& e) {
            throw PgnError("Move " + san_move + " at movenumber " + std::to_string(movenumber) + " isn't legal.");
        }

        if (game.live_position().turn() == Color::White) { movenumber++; }
    }

    game.record_unknown_result(pgn_data.result);
    
    return game;
}
