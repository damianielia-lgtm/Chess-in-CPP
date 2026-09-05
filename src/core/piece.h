#pragma once

#include <array>
#include <cassert>
#include <string_view>
#include <cstdint>

/*
5 bit move encoding
bits 0-4: piece type (type PieceType)
bit 5: color (type Color)
*/

enum class Color : std::uint8_t {
    White = 0,
    Black = 1
};

enum class PieceType : std::uint8_t {
    Empty = 0,
    Pawn = 1,
    Knight = 2,
    Bishop = 3,
    Rook = 4,
    Queen = 5,
    King = 6
};

class Piece {
public:
    Piece() noexcept = default;

    Piece(Color color, PieceType type) noexcept {
        assert(type != PieceType::Empty);
        encoding_ = static_cast<std::uint8_t>(type);
        encoding_ |= static_cast<std::uint8_t>(color) << 3;
    }

    constexpr explicit Piece(char symbol) noexcept {
        assert(static_cast<std::string_view>("PNBRQKpnbrqk.").contains(symbol));
        switch (symbol) {
            case 'P': encoding_ = 1; break;
            case 'N': encoding_ = 2; break;
            case 'B': encoding_ = 3; break;
            case 'R': encoding_ = 4; break;
            case 'Q': encoding_ = 5; break;
            case 'K': encoding_ = 6; break;
            
            case 'p': encoding_ = 9; break;
            case 'n': encoding_ = 10; break;
            case 'b': encoding_ = 11; break;
            case 'r': encoding_ = 12; break;
            case 'q': encoding_ = 13; break;
            case 'k': encoding_ = 14; break;
            
            case '.': encoding_ = 0; break;
        }
    }

    bool empty() const noexcept { return static_cast<PieceType>(encoding_ & 7) == PieceType::Empty; }
    Color color() const noexcept {
        assert(!empty());
        return static_cast<Color>(encoding_ >> 3);
    }
    constexpr PieceType type() const noexcept { return static_cast<PieceType>(encoding_ & 7); }
    char symbol() const noexcept {
        switch (encoding_) {
            case 1: return 'P';
            case 2: return 'N';
            case 3: return 'B';
            case 4: return 'R';
            case 5: return 'Q';
            case 6: return 'K';
            
            case 9: return 'p';
            case 10: return 'n';
            case 11: return 'b';
            case 12: return 'r';
            case 13: return 'q';
            case 14: return 'k';

            case 0: return '.';
        }
    }
    int material_value() const noexcept {
        switch (encoding_) {
            case 1: return 1;
            case 2: return 3;
            case 3: return 3;
            case 4: return 5;
            case 5: return 9;
            
            case 9: return -1;
            case 10: return -3;
            case 11: return -3;
            case 12: return -5;
            case 13: return -9;

            case 0: return 0;
            case 6: return 0;
            case 14: return 0;
        }
    }
    bool is_enemy(Color turn) const noexcept {
        assert(!empty());
        return color() != turn;
    }
    bool is_own(Color turn) const noexcept {
        assert(!empty());
        return color() == turn;
    }

    constexpr std::uint8_t encoding() const noexcept { return encoding_; }

    bool operator==(const Piece&) const = default;

private:
    std::uint8_t encoding_ = 0;
};

static_assert(sizeof(Piece) == 1);

constexpr std::array<Piece, 12> ALL_PIECES = {{
    Piece('P'), Piece('N'), Piece('B'), Piece('R'), Piece('Q'), Piece('K'),
    Piece('p'), Piece('n'), Piece('b'), Piece('r'), Piece('q'), Piece('k')
}};
