#include "pgn.h"

#include <string>

#include "../game/game.h"
#include "../errors.h"
#include "san.h"

Game reconstruct_game(const ParsedPGN& pgn_data) {
    Game game(pgn_data.white_name, pgn_data.black_name);

    int movenumber = 1;
    for (const std::string& san_move : pgn_data.san_moves) {
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
