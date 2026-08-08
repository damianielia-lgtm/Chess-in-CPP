#pragma once

#include <string>
#include <cstdint>

#include "square.h"
#include "piece.h"

/*
16 bit move encoding
bits 0-5: origin index (type Square)
bits 6-11: target index (type Square)
bits 12-15: move kind (type MoveKind)
*/

enum class MoveKind : std::uint8_t {
    None = 0,
    Capture = 1,
    DoublePawn = 2,
    Castling = 3,
    EnPassant = 4,

    KnightPromotion = 5,
    BishopPromotion = 6,
    RookPromotion = 7,
    QueenPromotion = 8,

    KnightPromotionCapture = 9,
    BishopPromotionCapture = 10,
    RookPromotionCapture = 11,
    QueenPromotionCapture = 12,

    // 3 values remain available.
};

class Move {
public:
    Move() noexcept = default;

    constexpr explicit Move(Square origin, Square target, MoveKind flags) noexcept {
        encoding_ = origin.index();
        encoding_ |= target.index() << 6;
        encoding_ |= static_cast<std::uint8_t>(flags) << 12;
    }

    std::string to_uci() const;

    Square origin() const noexcept { return Square(encoding_ & 63); }
    Square target() const noexcept { return Square((encoding_ >> 6) & 63); }
    Square en_passant_capture() const noexcept { return Square(((encoding_ >> 6) & 7) | (encoding_ & 56)); }
    MoveKind kind() const noexcept { return static_cast<MoveKind>(encoding_ >> 12); }

    PieceType promotion_type() const noexcept {
        switch (kind()) {
            case MoveKind::KnightPromotion: return PieceType::Knight;
            case MoveKind::KnightPromotionCapture: return PieceType::Knight;
            case MoveKind::BishopPromotion: return PieceType::Bishop;
            case MoveKind::BishopPromotionCapture: return PieceType::Bishop;
            case MoveKind::RookPromotion: return PieceType::Rook;
            case MoveKind::RookPromotionCapture: return PieceType::Rook;
            case MoveKind::QueenPromotion: return PieceType::Queen;
            case MoveKind::QueenPromotionCapture: return PieceType::Queen;
            default: return PieceType::Empty;
        }
    }

    bool is_capture() const noexcept {
        switch (kind()) {
            case MoveKind::Capture: return true;
            case MoveKind::EnPassant: return true;
            case MoveKind::KnightPromotionCapture: return true;
            case MoveKind::BishopPromotionCapture: return true;
            case MoveKind::RookPromotionCapture: return true;
            case MoveKind::QueenPromotionCapture: return true;
            default: return false;
        }
    }
    bool is_double_pawn() const noexcept { return kind() == MoveKind::DoublePawn; }
    bool is_en_passant() const noexcept { return kind() == MoveKind::EnPassant; }
    bool is_castling() const noexcept { return kind() == MoveKind::Castling; }
    bool is_promotion() const noexcept { return kind() > MoveKind::EnPassant; }
    bool is_loud() const noexcept {
        switch (kind()) {
            case MoveKind::None: return false;
            case MoveKind::DoublePawn: return false;
            case MoveKind::Castling: return false;
            default: return true;
        }
    }

private:
    std::uint16_t encoding_ = 0;
};

static_assert(sizeof(Move) == 2);
