#include "attacks.h"

#include "../core/square.h"
#include "../core/position.h"
#include "../core/piece.h"
#include "../core/move.h"
#include "move_tables.h"

bool is_attacked_square(
    const Position& position,
    const Square square, 
    const Color attacking_color
) {
    for (const Square attack : TABLES.PAWN_ATTACKS[static_cast<uint8_t>(attacking_color) ^ 1][square.index()]) {
        if (position.piece_at(attack) == Piece(attacking_color, PieceType::Pawn)) {
            return true;
        }
    }
    for (const Square attack : TABLES.KNIGHT_MOVEMENT[square.index()]) {
        if (position.piece_at(attack) == Piece(attacking_color, PieceType::Knight)) {
            return true;
        }
    }
    for (const Square attack : TABLES.KING_MOVEMENT[square.index()]) {
        if (position.piece_at(attack) == Piece(attacking_color, PieceType::King)) {
            return true;
        }
    }

    for (const Targets<7>& ray : TABLES.BISHOP_MOVEMENT[square.index()]) {
        for (const Square attack : ray) {
            Piece piece = position.piece_at(attack);
            if (piece == Piece(attacking_color, PieceType::Bishop) ||
                piece == Piece(attacking_color, PieceType::Queen)) {
                return true;
            }
            if (!piece.empty()) {
                break;
            }
        }
    }
    for (const Targets<7>& ray : TABLES.ROOK_MOVEMENT[square.index()]) {
        for (const Square attack : ray) {
            Piece piece = position.piece_at(attack);
            if (piece == Piece(attacking_color, PieceType::Rook) ||
                piece == Piece(attacking_color, PieceType::Queen)) {
                return true;
            }
            if (!piece.empty()) {
                break;
            }
        }
    }

    return false;
}

bool is_legal_move(Position& position, const Move move) {
    const Piece capture = move.is_en_passant()
        ? position.piece_at(move.en_passant_capture())
        : position.piece_at(move.target());

    const Square king = (position.piece_at(move.origin()).type() == PieceType::King)
        ? move.target()
        : position.king_square(position.turn());

    position.make_move(move);

    if (is_attacked_square(position, king, position.opposite_turn())) {
        position.unmake_move(move, capture);
        return false;
    }
    
    position.unmake_move(move, capture);
    return true;
}
