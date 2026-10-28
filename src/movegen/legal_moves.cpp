#include "legal_moves.h"

#include <array>
#include <algorithm>
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

int score_quiet(std::uint8_t phase, Piece piece, Move move) {
    return (move.is_castling()
        ? castling_scores[phase][CastlingOption(move).index()]
        : tampered_scores[phase][piece.encoding()][move.target().index()]
           - tampered_scores[phase][piece.encoding()][move.origin().index()]);
}

void castling(const Position& position, const CastlingOption option, auto& emit) {
    static constexpr std::array<Targets<3>, 4> CastlingEmpty = {{
        {Square('f', '1'), Square('g', '1')},
        {Square('b', '1'), Square('c', '1'), Square('d', '1')},
        {Square('f', '8'), Square('g', '8')},
        {Square('b', '8'), Square('c', '8'), Square('d', '8')}
    }};

    if (!position.castling_rights().has(option)) { return; }

    for (const Square square : CastlingEmpty[option.index()]) {
        if (!position.piece_at(square).empty()) { return; }
    }

    emit(option.castling_king_movement());
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

void generate_quiets(Position& position, auto& emit) {
    const Color us = position.turn();

    for (const Square square : Square::all()) {
        const Piece piece = position.piece_at(square);
        if (piece.empty() || piece.is_enemy(us)) { continue; }

        pseudo_quiets(square, piece, position, emit);
    }

    for (
        const CastlingOption& option : {
            CastlingOption(us, CastlingSide::Kingside),
            CastlingOption(us, CastlingSide::Queenside)
        }
    ) {
        castling(position, option, emit);
    }
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

void generate_captures(Position& position, auto& emit) {
    const Color us = position.turn();

    for (const Square square : Square::all()) {
        const Piece piece = position.piece_at(square);
        if (piece.empty() || piece.is_enemy(us)) { continue; }

        pseudo_captures(square, piece, position, emit);
    }
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

void generate_promotions(Position& position, auto& emit) {
    for (
        const Square square :
        (position.turn() == Color::White ? Square::iterate(48, 55) : Square::iterate(8, 15))
    ) {
        if (position.piece_at(square) == Piece(position.turn(), PieceType::Pawn)) {
            pseudo_promotions(square, position, emit);
        }
    }
}

}

Move MovePicker::next_move() {
    auto emit_promotion = [&](Move move) {
        assert(count_ < capacity);
        int score = score_promotion(
            move.promotion_type(),
            position_.piece_at(move.origin()),
            position_.piece_at(move.target())
        );
        moves_[count_++] = ScoredMove{move, score};
    };

    auto emit_capture = [&](Move move) {
        assert(count_ < capacity);
        int score = score_capture(
            position_.piece_at(move.origin()),
            move.is_en_passant()
                ? position_.piece_at(move.en_passant_capture())
                : position_.piece_at(move.target())
        );
        moves_[count_++] = ScoredMove{move, score};
    };

    auto emit_quiet = [&](Move move) {
        assert(count_ < capacity);
        int score = score_quiet(
            position_.phase(),
            position_.piece_at(move.origin()),
            move
        );
        moves_[count_++] = ScoredMove{move, score};
    };

    if (count_ == 0) {
        sorted_ = false;
        switch (stage_) {
            case GenerationStage::NotStarted:
                stage_ = GenerationStage::Promotions;
                generate_promotions(position_, emit_promotion);
                return next_move();
            case GenerationStage::Promotions:
                stage_ = GenerationStage::Captures;
                generate_captures(position_, emit_capture);
                return next_move();
            case GenerationStage::Captures:
                stage_ = GenerationStage::Quiets;
                generate_quiets(position_, emit_quiet);
                return next_move();
            case GenerationStage::Quiets:
                return Move::none();
        }
    } else {
        Move best_pseudo;

        if (first_move_) {
            auto best_it = std::max_element(
                moves_.begin(), moves_.begin() + count_,
                [](const ScoredMove& a, const ScoredMove& b) {
                    return a.score < b.score;
                }
            );

            best_pseudo = best_it->move;

            *best_it = moves_[--count_];
        } else {
            if (!sorted_) {
                std::sort(
                    moves_.begin(), moves_.begin() + count_,
                    [](const ScoredMove& a, const ScoredMove& b) {
                        return a.score < b.score; // ascending: best move ends up last
                    }
                );
                sorted_ = true;
            }

            count_--;
            best_pseudo = moves_[count_].move;
        }

        if (!is_legal_move(position_, best_pseudo)) {
            return next_move();
        }

        first_move_ = false;
        return best_pseudo;
    }
}

void generate_all_moves(MovesList& legal_moves, Position& position) {
    legal_moves.clear();

    auto emit_if_legal = [&](Move move) {
        if (is_legal_move(position, move)) {
            legal_moves.push(move);
        }
    };

    generate_promotions(position, emit_if_legal);
    generate_captures(position, emit_if_legal);
    generate_quiets(position, emit_if_legal);
}

MovesList all_moves(Position& position) {
    MovesList legal_moves;
    generate_all_moves(legal_moves, position);
    return legal_moves;
}
