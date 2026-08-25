#include <string>
#include <string_view>

#include "../core/move.h"
#include "../core/piece.h"
#include "../core/position.h"
#include "../movegen/legal_moves.h"
#include "../errors.h"

Move resolve_uci(const Position& position, const std::string_view uci) {
    Position modifiable_copy = position;
    for (const Move move : all_moves(modifiable_copy, MoveGeneration::All)) {
        if (move.to_uci() == uci) {
            return move;
        }
    }

    throw IllegalMoveError(std::string(uci) + " is not a legal move on the current position.");
}

std::string Move::to_uci() const {
    std::string uci;

    uci += origin().uci_file();
    uci += origin().uci_rank();
    uci += target().uci_file();
    uci += target().uci_rank();

    if (is_promotion()) {
        uci += Piece(Color::Black, promotion_type()).symbol();
    }
    
    return uci;
}
