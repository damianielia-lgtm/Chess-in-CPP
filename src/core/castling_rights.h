#pragma once

#include <string>
#include <cstdint>
#include <array>
#include <cassert>

#include "piece.h"
#include "move.h"
#include "square.h"

enum class CastlingType : std::uint8_t {
    WhiteKingside = 0,
    WhiteQueenside = 1,
    BlackKingside = 2,
    BlackQueenside = 3
};

enum class CastlingSide : std::uint8_t {
    Kingside = 0,
    Queenside = 1
};

class CastlingOption {
public:
    explicit CastlingOption(CastlingType type) noexcept : encoding_(type) {}

    CastlingOption(Color color, CastlingSide side) noexcept {
        encoding_ = static_cast<CastlingType>((static_cast<std::uint8_t>(color) << 1) | static_cast<std::uint8_t>(side));
    }

    explicit CastlingOption(char fen_char) noexcept {
        assert(static_cast<std::string>("KQkq").contains(fen_char));
        switch (fen_char) {
            case 'K': encoding_ = CastlingType::WhiteKingside; break;
            case 'Q': encoding_ = CastlingType::WhiteQueenside; break;
            case 'k': encoding_ = CastlingType::BlackKingside; break;
            case 'q': encoding_ = CastlingType::BlackQueenside; break;
        }
    }

    explicit CastlingOption(Move move) noexcept
        : CastlingOption(
            (move.origin().rank() == 0) ? Color::White : Color::Black,
            (move.target().file() == 6) ? CastlingSide::Kingside : CastlingSide::Queenside) {
        assert(move.is_castling());
    }

    [[nodiscard]] CastlingType encoding() const noexcept { return encoding_; }
    [[nodiscard]] std::uint8_t index() const noexcept { return static_cast<std::uint8_t>(encoding_); }
    [[nodiscard]] Color color() const noexcept { return static_cast<Color>(index() >> 1); }
    [[nodiscard]] CastlingSide side() const noexcept { return static_cast<CastlingSide>(index() & 1); }

    [[nodiscard]] Move castling_king_movement() const noexcept {
        switch (encoding_) {
            case CastlingType::WhiteKingside: return Move(Square('e', '1'), Square('g', '1'), MoveKind::Castling);
            case CastlingType::WhiteQueenside: return Move(Square('e', '1'), Square('c', '1'), MoveKind::Castling);
            case CastlingType::BlackKingside: return Move(Square('e', '8'), Square('g', '8'), MoveKind::Castling);
            case CastlingType::BlackQueenside: return Move(Square('e', '8'), Square('c', '8'), MoveKind::Castling);
        }
        return Move(Square('e', '1'), Square('g', '1'), MoveKind::Castling);
    }
    [[nodiscard]] Move castling_rook_movement() const noexcept {
        switch (encoding_) {
            case CastlingType::WhiteKingside: return Move(Square('h', '1'), Square('f', '1'), MoveKind::None);
            case CastlingType::WhiteQueenside: return Move(Square('a', '1'), Square('d', '1'), MoveKind::None);
            case CastlingType::BlackKingside: return Move(Square('h', '8'), Square('f', '8'), MoveKind::None);
            case CastlingType::BlackQueenside: return Move(Square('a', '8'), Square('d', '8'), MoveKind::None);
        }
        return Move(Square('h', '1'), Square('f', '1'), MoveKind::None);
    }

    [[nodiscard]] char fen_char() const noexcept {
        switch (encoding_) {
            case CastlingType::WhiteKingside: return 'K';
            case CastlingType::WhiteQueenside: return 'Q';
            case CastlingType::BlackKingside: return 'k';
            case CastlingType::BlackQueenside: return 'q';
        }
        return '?';
    }

    static std::array<CastlingOption, 4> all() noexcept {
        return {
            CastlingOption(CastlingType::WhiteKingside), CastlingOption(CastlingType::WhiteQueenside),
            CastlingOption(CastlingType::BlackKingside), CastlingOption(CastlingType::BlackQueenside)
        };
    }

private:
    CastlingType encoding_;
};

class CastlingRights {
public:
    CastlingRights() noexcept = default;

    [[nodiscard]] std::uint8_t encoding() const noexcept { return encoding_; }

    [[nodiscard]] bool has(CastlingOption index) const noexcept { return (encoding_ >> index.index()) & 1; }

    void revoke(CastlingOption option) noexcept {
        const auto bit = std::uint8_t{1} << option.index();
        encoding_ &= static_cast<std::uint8_t>(~bit);
    }
    void revoke_all() noexcept { encoding_ = 0; }
    void revoke_from_rook(Square square) noexcept {
        if (square == Square('h', '1')) {
            revoke(CastlingOption(CastlingType::WhiteKingside));
        } else if (square == Square('a', '1')) {
            revoke(CastlingOption(CastlingType::WhiteQueenside));
        } else if (square == Square('h', '8')) {
            revoke(CastlingOption(CastlingType::BlackKingside));
        } else if (square == Square('a', '8')) {
            revoke(CastlingOption(CastlingType::BlackQueenside));
        }
    }

    void grant(CastlingOption option) noexcept {
        const auto bit = std::uint8_t{1} << option.index();
        encoding_ |= bit;
    }
    void grant_all() noexcept { encoding_ = 15; }

    bool operator==(const CastlingRights&) const = default;

private:
    std::uint8_t encoding_ = 0;
};

static_assert(sizeof(CastlingRights) == 1);
