#include "legal_moves.h"

#include <array>
#include <cstdint>

#include "../core/piece.h"
#include "../core/move.h"
#include "../core/square.h"
#include "../core/position.h"
#include "../engine/evaluation.h"
#include "move_tables.h"
#include "attacks.h"

namespace {

std::uint8_t side_index(const Position& position) {
    return static_cast<std::uint8_t>(position.turn());
}

bool is_enemy_piece(const Position& position, Piece piece) {
    return !piece.empty() && piece.is_enemy(position.turn());
}

// First or last rank: a pawn arriving here promotes.
bool is_back_rank(Square square) {
    return square.index() < 8 || square.index() > 55;
}

void for_each_step(const Targets<8>& steps, const Position& position, auto&& visit) {
    for (const Square target : steps) {
        visit(target, position.piece_at(target));
    }
}

void for_each_ray_square(const std::array<Targets<7>, 4>& rays, const Position& position, auto&& visit) {
    for (const Targets<7>& ray : rays) {
        for (const Square target : ray) {
            const Piece occupant = position.piece_at(target);
            visit(target, occupant);
            if (!occupant.empty()) { break; }
        }
    }
}

void for_each_reachable(Square origin, PieceType type, const Position& position, auto&& visit) {
    const auto i = origin.index();
    switch (type) {
        case PieceType::Knight:
            for_each_step(TABLES.KNIGHT_MOVEMENT[i], position, visit);
            break;
        case PieceType::Bishop:
            for_each_ray_square(TABLES.BISHOP_MOVEMENT[i], position, visit);
            break;
        case PieceType::Rook:
            for_each_ray_square(TABLES.ROOK_MOVEMENT[i], position, visit);
            break;
        case PieceType::Queen:
            for_each_ray_square(TABLES.BISHOP_MOVEMENT[i], position, visit);
            for_each_ray_square(TABLES.ROOK_MOVEMENT[i], position, visit);
            break;
        case PieceType::King:
            for_each_step(TABLES.KING_MOVEMENT[i], position, visit);
            break;
        case PieceType::Pawn:
            break; // Pawns have their own generators.
    }
}

/* --- QUIETS --- */

int score_quiet(std::uint8_t phase, Piece piece, Square origin, Square target) {
    return tampered_scores[phase][piece.encoding()][target.index()]
        - tampered_scores[phase][piece.encoding()][origin.index()];
}

void pseudo_pawn_quiets(Square origin, const Position& position, auto& emit) {
    const Piece pawn(position.turn(), PieceType::Pawn);
    const std::uint8_t side = side_index(position);

    const Square push = TABLES.PAWN_PUSH[side][origin.index()];
    if (position.piece_at(push).empty() && !is_back_rank(push)) {
        emit(Move(origin, push, MoveKind::None));
    }

    const DoublePawnSquares& double_push = TABLES.DOUBLE_PAWN_PUSH[side][origin.index()];
    if (
        double_push.available &&
        position.piece_at(double_push.intermediate).empty() &&
        position.piece_at(double_push.target).empty()
    ) {
        emit(Move(origin, double_push.target, MoveKind::DoublePawn));
    }
}

void pseudo_quiets(Square origin, Piece piece, const Position& position, auto& emit) {
    if (piece.type() == PieceType::Pawn) {
        pseudo_pawn_quiets(origin, position, emit);
        return;
    }

    for_each_reachable(
        origin, piece.type(), position,
        [&](Square target, Piece occupant) {
            if (occupant.empty()) {
                emit(Move(origin, target, MoveKind::None));
            }
        }
    );
}

/* --- CAPTURES --- */

int score_capture(Piece attacker, Piece victim) {
    return mvvlva[attacker.encoding()][victim.encoding()];
}

void pseudo_pawn_captures(Square origin, const Position& position, auto& emit) {
    const Piece pawn(position.turn(), PieceType::Pawn);

    for (const Square target : TABLES.PAWN_ATTACKS[side_index(position)][origin.index()]) {
        if (target == position.en_passant_target()) {
            const Piece victim(position.opposite_turn(), PieceType::Pawn);
            emit(Move(origin, target, MoveKind::EnPassant));
            continue;
        }

        const Piece victim = position.piece_at(target);
        if (is_enemy_piece(position, victim) && !is_back_rank(target)) {
            emit(Move(origin, target, MoveKind::Capture));
        }
    }
}

void pseudo_captures(Square origin, Piece piece, const Position& position, auto& emit) {
    if (piece.type() == PieceType::Pawn) {
        pseudo_pawn_captures(origin, position, emit);
        return;
    }

    for_each_reachable(
        origin, piece.type(),
        position, [&](Square target, Piece occupant) {
            if (is_enemy_piece(position, occupant)) {
                emit(Move(origin, target, MoveKind::Capture));
            }
        }
    );
}

/* --- PROMOTIONS --- */

int score_promotion(
    PieceType promotion_type,
    Piece moving_piece,
    Piece target_piece
) {
    int score;
    switch (promotion_type) {
        case PieceType::Queen:
            score = 10000;
            break;
        case PieceType::Rook:
            score = 9000;
            break;
        default:
            score = 8500;
    }
    if (!target_piece.empty()) {
        score += score_capture(moving_piece, target_piece);
    }
    return score;
}

void pseudo_promotions(Square origin, const Position& position, auto& emit) {
    const Piece pawn(position.turn(), PieceType::Pawn);
    const std::uint8_t side = side_index(position);

    for (const Square target : TABLES.PAWN_ATTACKS[side][origin.index()]) {
        const Piece victim = position.piece_at(target);
        if (!is_enemy_piece(position, victim)) { continue; }
        emit(Move(origin, target, MoveKind::QueenPromotionCapture));
        emit(Move(origin, target, MoveKind::RookPromotionCapture));
        emit(Move(origin, target, MoveKind::BishopPromotionCapture));
        emit(Move(origin, target, MoveKind::KnightPromotionCapture));
    }

    const Square push = TABLES.PAWN_PUSH[side][origin.index()];
    if (position.piece_at(push).empty()) {
        emit(Move(origin, push, MoveKind::QueenPromotion));
        emit(Move(origin, push, MoveKind::RookPromotion));
        emit(Move(origin, push, MoveKind::BishopPromotion));
        emit(Move(origin, push, MoveKind::KnightPromotion));
    }
}

/* --- CASTLING --- */

constexpr std::array<Targets<3>, 4> CastlingEmpty = {{
    {Square('f', '1'), Square('g', '1')},
    {Square('b', '1'), Square('c', '1'), Square('d', '1')},
    {Square('f', '8'), Square('g', '8')},
    {Square('b', '8'), Square('c', '8'), Square('d', '8')}
}};

constexpr std::array<Targets<3>, 4> CastlingSafe = {{
    {Square('e', '1'), Square('f', '1'), Square('g', '1')},
    {Square('e', '1'), Square('d', '1'), Square('c', '1')},
    {Square('e', '8'), Square('f', '8'), Square('g', '8')},
    {Square('e', '8'), Square('d', '8'), Square('c', '8')}
}};

bool can_castle(const Position& position, const CastlingOption option) {
    if (!position.castling_rights().has(option)) { return false; }

    for (const Square square : CastlingEmpty[option.index()]) {
        if (!position.piece_at(square).empty()) { return false; }
    }

    for (const Square square : CastlingSafe[option.index()]) {
        if (is_attacked_square(position, square, position.opposite_turn())) { return false; }
    }

    return true;
}

}

void generate_scored_moves(ScoredMoveList& legal_moves, Position& position) {
    legal_moves.clear();

    const Color us = position.turn();
    const std::uint8_t phase = get_phase(position);

    auto emit_promotion = [&](Move move) {
        if (!is_legal_move(position, move)) {
            return;
        }
        
        int score = score_promotion(
            move.promotion_type(),
            position.piece_at(move.origin()),
            position.piece_at(move.target())
        );
        legal_moves.push(ScoredMove{move, score});
    };

    for (const Square square : (us == Color::White ? Square::iterate(48, 55) : Square::iterate(8, 15))) {
        if (position.piece_at(square) == Piece(us, PieceType::Pawn)) {
            pseudo_promotions(square, position, emit_promotion);
        }
    }

    auto emit_capture = [&](Move move) {
        if (!is_legal_move(position, move)) {
            return;
        }
        
        int score = score_capture(
            position.piece_at(move.origin()),
            move.is_en_passant()
                ? position.piece_at(move.en_passant_capture())
                : position.piece_at(move.target())
        );
        legal_moves.push(ScoredMove{move, score});
    };

    auto emit_quiet = [&](Move move) {
        if (!is_legal_move(position, move)) {
            return;
        }
        
        int score = score_quiet(
            phase,
            position.piece_at(move.origin()),
            move.origin(),
            move.target()
        );
        legal_moves.push(ScoredMove{move, score});
    };

    for (const Square square : Square::all()) {
        const Piece piece = position.piece_at(square);
        if (piece.empty() || piece.is_enemy(us)) { continue; }

        pseudo_captures(square, piece, position, emit_capture);
        pseudo_quiets(square, piece, position, emit_quiet);
    }

    for (
        const CastlingOption& option : {
            CastlingOption(us, CastlingSide::Kingside),
            CastlingOption(us, CastlingSide::Queenside)
        }
    ) {
        if (!can_castle(position, option)) { continue; }
        legal_moves.push(
            ScoredMove{option.castling_king_movement(),
                castling_scores[phase][option.index()]}
        );
    }
}

void generate_all_moves(MovesList& legal_moves, Position& position) {
    legal_moves.clear();

    const Color us = position.turn();
    const std::uint8_t phase = get_phase(position);

    auto emit_if_legal = [&](Move move) {
        if (is_legal_move(position, move)) {
            legal_moves.push(move);
        }
    };

    for (const Square square : (us == Color::White ? Square::iterate(48, 55) : Square::iterate(8, 15))) {
        if (position.piece_at(square) == Piece(us, PieceType::Pawn)) {
            pseudo_promotions(square, position, emit_if_legal);
        }
    }

    for (const Square square : Square::all()) {
        const Piece piece = position.piece_at(square);
        if (piece.empty() || piece.is_enemy(us)) { continue; }

        pseudo_captures(square, piece, position, emit_if_legal);
        pseudo_quiets(square, piece, position, emit_if_legal);
    }

    for (
        const CastlingOption& option : {
            CastlingOption(us, CastlingSide::Kingside),
            CastlingOption(us, CastlingSide::Queenside)
        }
    ) {
        if (!can_castle(position, option)) { continue; }
        legal_moves.push(option.castling_king_movement());
    }
}

MovesList all_moves(Position& position) {
    MovesList legal_moves;
    generate_all_moves(legal_moves, position);
    return legal_moves;
}
