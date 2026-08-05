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
    Square origin,
    const Position& position,
    bool loud,
    auto& emit
) {
    for (const Square target : TABLES.PAWN_ATTACKS[static_cast<std::uint8_t>(position.turn())][origin.index()]) {
        Piece piece = position.piece_at(target);
        if (target == position.en_passant_target()) {
            emit(Move(origin, target, MoveKind::EnPassant));
        } else if (!piece.empty() && piece.is_enemy(position.turn())) {
            if (7 >= target.index() || target.index() >= 56) {
                emit(Move(origin, target, MoveKind::KnightPromotionCapture));
                emit(Move(origin, target, MoveKind::BishopPromotionCapture));
                emit(Move(origin, target, MoveKind::RookPromotionCapture));
                emit(Move(origin, target, MoveKind::QueenPromotionCapture));
            } else {
                emit(Move(origin, target, MoveKind::Capture));
            }
        }
    }

    Square target = TABLES.PAWN_PUSH[static_cast<std::uint8_t>(position.turn())][origin.index()];
    if (position.piece_at(target).empty()) {
        if (7 >= target.index() || target.index() >= 56) {
            emit(Move(origin, target, MoveKind::KnightPromotion));
            emit(Move(origin, target, MoveKind::BishopPromotion));
            emit(Move(origin, target, MoveKind::RookPromotion));
            emit(Move(origin, target, MoveKind::QueenPromotion));
        } else {
            if (!loud) {
                emit(Move(origin, target, MoveKind::None));
            }
        }
    }

    const DoublePawnSquares& double_pawn_push = TABLES.DOUBLE_PAWN_PUSH[static_cast<std::uint8_t>(position.turn())][origin.index()];
    if (double_pawn_push.available && position.piece_at(double_pawn_push.intermediate).empty() && position.piece_at(double_pawn_push.target).empty()) {
        if (!loud) { emit(Move(origin, double_pawn_push.target, MoveKind::DoublePawn)); }
    }
}

void pseudo_sliding_moves(
    Square origin,
    const Position& position,
    bool loud,
    const std::array<Targets<7>, 4>& table,
    auto& emit
) {
    for (const Targets<7>& ray : table) {
        for (const Square target_index : ray) {
            Piece piece = position.piece_at(target_index);
            if (piece.empty()) {
                if (!loud) {
                    emit(Move(origin, target_index, MoveKind::None));
                }
                continue;
            } else if (piece.is_enemy(position.turn())) {
                emit(Move(origin, target_index, MoveKind::Capture));
                break;
            } else {
                break;
            }
        }
    }
}

void pseudo_piece_moves(
    Square origin,
    const Position& position,
    bool loud,
    const Targets<8>& table,
    auto& emit
) {
    for (const Square target_index : table) {
        Piece piece = position.piece_at(target_index);
        if (piece.empty()) {
            if (!loud) {
                emit(Move(origin, target_index, MoveKind::None));
            }
        } else if (piece.is_enemy(position.turn())) {
            emit(Move(origin, target_index, MoveKind::Capture));
        }
    }
}

void pseudo_moves(
    Square origin,
    const PieceType piece_moved,
    const Position& position,
    bool loud,
    auto& emit
) {
    switch (piece_moved) {
    case PieceType::Pawn:
        pseudo_pawn_moves(origin, position, loud, emit);
        break;
    case PieceType::Knight:
        pseudo_piece_moves(origin, position, loud, TABLES.KNIGHT_MOVEMENT[origin.index()], emit);
        break;
    case PieceType::Bishop:
        pseudo_sliding_moves(origin, position, loud, TABLES.BISHOP_MOVEMENT[origin.index()], emit);
        break;
    case PieceType::Rook:
        pseudo_sliding_moves(origin, position, loud, TABLES.ROOK_MOVEMENT[origin.index()], emit);
        break;
    case PieceType::Queen:
        pseudo_sliding_moves(origin, position, loud, TABLES.BISHOP_MOVEMENT[origin.index()], emit);
        pseudo_sliding_moves(origin, position, loud, TABLES.ROOK_MOVEMENT[origin.index()], emit);
        break;
    case PieceType::King:
        pseudo_piece_moves(origin, position, loud, TABLES.KING_MOVEMENT[origin.index()], emit);
        break;
    }
}

MovesList all_legal_moves(Position& position, bool loud) {
    MovesList legal_moves;

    auto emit_if_legal = [&position, &legal_moves](Move move) {
        if (is_legal_move(position, move)) {
            legal_moves.push(move);
        }
    };

    for (const Square square : Square::all()) {
        Piece piece = position.piece_at(square);

        if (piece.empty() || piece.is_enemy(position.turn())) {
            continue;
        }
        
        pseudo_moves(square, piece.type(), position, loud, emit_if_legal);
    }
    
    if (!loud) {
        CastlingOption castling_index(position.turn(), CastlingSide::Kingside);

        if (can_castle(position, castling_index)) {
            legal_moves.push(castling_index.castling_king_movement());
        }
        castling_index = CastlingOption(position.turn(), CastlingSide::Queenside);
        if (can_castle(position, castling_index)) {
            legal_moves.push(castling_index.castling_king_movement());
        }
    }
    return legal_moves;
}
