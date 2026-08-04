#include <stdexcept>
#include <string>

#include "../core/move.h"
#include "../core/piece.h"
#include "../core/position.h"
#include "../movegen/legal_moves.h"

Move Position::resolve_uci(const std::string_view uci) {
    for (const Move move : all_legal_moves(*this, false)) {
        if (move.to_uci() == uci) {
            return move;
        }
    }
    throw std::invalid_argument(std::string(uci) + " not found in legal moves list.");
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
