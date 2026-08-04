#include "legal_moves.h"

#include "../core/piece.h"
#include "../core/move.h"
#include "../core/move_list.h"
#include "../core/square.h"
#include "../core/position.h"
#include "move_tables.h"
#include "castling.h"
#include "attacks.h"

void pseudo_pawn_moves(
    MovesList& moves,
    Square square,
    const Position& position,
    bool loud
) {
    for (const Square target : TABLES.PAWN_ATTACKS[static_cast<std::uint8_t>(position.turn())][square.index()]) {
        Piece piece = position.piece_at(target);
        if (target == position.en_passant_target()) {
            moves.push(Move(square, target, MoveKind::EnPassant));
        } else if (!piece.empty() && piece.is_enemy(position.turn())) {
            if (7 >= target.index() || target.index() >= 56) {
                moves.push(Move(square, target, MoveKind::KnightPromotionCapture));
                moves.push(Move(square, target, MoveKind::BishopPromotionCapture));
                moves.push(Move(square, target, MoveKind::RookPromotionCapture));
                moves.push(Move(square, target, MoveKind::QueenPromotionCapture));
            } else {
                moves.push(Move(square, target, MoveKind::Capture));
            }
        }
    }

    Square target = TABLES.PAWN_PUSH[static_cast<std::uint8_t>(position.turn())][square.index()];
    if (position.piece_at(target).empty()) {
        if (7 >= target.index() || target.index() >= 56) {
            moves.push(Move(square, target, MoveKind::KnightPromotion));
            moves.push(Move(square, target, MoveKind::BishopPromotion));
            moves.push(Move(square, target, MoveKind::RookPromotion));
            moves.push(Move(square, target, MoveKind::QueenPromotion));
        } else {
            if (!loud) {
                moves.push(Move(square, target, MoveKind::None));
            }
        }
    }

    const DoublePawnSquares& double_pawn_push = TABLES.DOUBLE_PAWN_PUSH[static_cast<std::uint8_t>(position.turn())][square.index()];
    if (double_pawn_push.available && position.piece_at(double_pawn_push.intermediate).empty() && position.piece_at(double_pawn_push.target).empty()) {
        if (!loud) { moves.push(Move(square, double_pawn_push.target, MoveKind::DoublePawn)); }
    }
}

void pseudo_sliding_moves(
    MovesList& moves,
    Square square,
    const Position& position,
    bool loud,
    const std::array<Targets<7>, 4>& table
) {
    for (const Targets<7>& ray : table) {
        for (const Square target_index : ray) {
            Piece piece = position.piece_at(target_index);
            if (piece.empty()) {
                if (!loud) {
                    moves.push(Move(square, target_index, MoveKind::None));
                }
                continue;
            } else if (piece.is_enemy(position.turn())) {
                moves.push(Move(square, target_index, MoveKind::Capture));
                break;
            } else {
                break;
            }
        }
    }
}

void pseudo_piece_moves(
    MovesList& moves,
    Square square,
    const Position& position,
    bool loud,const Targets<8>& table
) {
    for (const Square target_index : table) {
        Piece piece = position.piece_at(target_index);
        if (piece.empty()) {
            if (!loud) {
                moves.push(Move(square, target_index, MoveKind::None));
            }
        } else if (piece.is_enemy(position.turn())) {
            moves.push(Move(square, target_index, MoveKind::Capture));
        }
    }
}

MovesList pseudo_moves(
    Square square,
    const PieceType piece_moved,
    const Position& position,
    bool loud
) {
    MovesList moves;
    switch (piece_moved) {
    case PieceType::Pawn:
        pseudo_pawn_moves(moves, square, position, loud);
        break;
    case PieceType::Knight:
        pseudo_piece_moves(moves, square, position, loud, TABLES.KNIGHT_MOVEMENT[square.index()]);
        break;
    case PieceType::Bishop:
        pseudo_sliding_moves(moves, square, position, loud, TABLES.BISHOP_MOVEMENT[square.index()]);
        break;
    case PieceType::Rook:
        pseudo_sliding_moves(moves, square, position, loud, TABLES.ROOK_MOVEMENT[square.index()]);
        break;
    case PieceType::Queen:
        pseudo_sliding_moves(moves, square, position, loud, TABLES.BISHOP_MOVEMENT[square.index()]);
        pseudo_sliding_moves(moves, square, position, loud, TABLES.ROOK_MOVEMENT[square.index()]);
        break;
    case PieceType::King:
        pseudo_piece_moves(moves, square, position, loud, TABLES.KING_MOVEMENT[square.index()]);
        break;
    }
    return moves;
}

MovesList all_legal_moves(Position& position, bool loud) {
    MovesList moves_list;
    for (const Square square : Square::all()) {
        Piece piece = position.piece_at(square);
        if (!piece.empty() && piece.is_own(position.turn())) {
            for (const Move move : pseudo_moves(square, piece.type(), position, loud)) {
                if (is_legal_move(position, move)) {
                    moves_list.push(move);
                }
            }
        }
    }
    if (!loud) {
        CastlingOption castling_index(position.turn(), CastlingSide::Kingside);
        if (can_castle(position, castling_index)) {
            moves_list.push(castling_index.castling_king_movement());
        }
        castling_index = CastlingOption(position.turn(), CastlingSide::Queenside);
        if (can_castle(position, castling_index)) {
            moves_list.push(castling_index.castling_king_movement());
        }
    }
    return moves_list;
}
