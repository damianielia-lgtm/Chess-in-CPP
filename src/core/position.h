#pragma once

#include <array>
#include <cassert>
#include <string>
#include <string_view>

#include "square.h"
#include "move.h"
#include "piece.h"
#include "castling_rights.h"

struct UndoState {
    Piece captured_piece;
    CastlingRights castling_rights;
    Square en_passant_target;
    int halfmove_clock;
    int fullmove_number;
    Square white_king;
    Square black_king;
};

constexpr std::array<Piece, 64> STARTING_BOARD = {
    Piece('R'), Piece('N'), Piece('B'), Piece('Q'), Piece('K'), Piece('B'), Piece('N'), Piece('R'),
    Piece('P'), Piece('P'), Piece('P'), Piece('P'), Piece('P'), Piece('P'), Piece('P'), Piece('P'),
    Piece('.'), Piece('.'), Piece('.'), Piece('.'), Piece('.'), Piece('.'), Piece('.'), Piece('.'),
    Piece('.'), Piece('.'), Piece('.'), Piece('.'), Piece('.'), Piece('.'), Piece('.'), Piece('.'),
    Piece('.'), Piece('.'), Piece('.'), Piece('.'), Piece('.'), Piece('.'), Piece('.'), Piece('.'),
    Piece('.'), Piece('.'), Piece('.'), Piece('.'), Piece('.'), Piece('.'), Piece('.'), Piece('.'),
    Piece('p'), Piece('p'), Piece('p'), Piece('p'), Piece('p'), Piece('p'), Piece('p'), Piece('p'),
    Piece('r'), Piece('n'), Piece('b'), Piece('q'), Piece('k'), Piece('b'), Piece('n'), Piece('r')
};

class Position {
public:
    Position() noexcept { set_startpos(); }

    explicit Position(const std::string_view fen_string);
    std::string to_fen() const;

    Move resolve_uci(const std::string_view uci);

    Piece piece_at(Square index) const noexcept {
        assert(index.is_valid());
        return board_[index.index()];
    }

    Color turn() const noexcept { return turn_;};
    Color opposite_turn() const noexcept { return static_cast<Color>(static_cast<std::uint8_t>(turn_) ^ 1); }
    CastlingRights castling_rights() const noexcept { return castling_rights_; }
    Square en_passant_target() const noexcept { return en_passant_target_; }
    int halfmove_clock() const noexcept { return halfmove_clock_; }
    int fullmove_number() const noexcept { return fullmove_number_; }
    Square king_square(Color color) const noexcept { return (color == Color::White) ? white_king_ : black_king_; }

    void make_move(const Move move) noexcept;
    void unmake_move(const Move move, const Piece capture) noexcept;
    UndoState apply_move(const Move move) noexcept;
    void revert_move(const Move move, const UndoState& undo_state) noexcept;

    bool operator==(const Position& other) const {
        return (
            board_ == other.board_
            && turn_ == other.turn_
            && castling_rights_ == other.castling_rights_
            && en_passant_target_ == other.en_passant_target_
        );
    }

private:
    std::array<Piece, 64> board_;
    Color turn_;
    CastlingRights castling_rights_;
    Square en_passant_target_;
    int halfmove_clock_;
    int fullmove_number_;
    Square white_king_;
    Square black_king_;

    void set_piece(Square index, Piece piece) noexcept {
        assert(index.is_valid());
        board_[index.index()] = piece;
    }

    void clear_board() noexcept { board_.fill(Piece()); }

    void set_startpos() noexcept {
        board_ = STARTING_BOARD;
        turn_ = Color::White;
        castling_rights_.grant_all();
        en_passant_target_ = Square();
        halfmove_clock_ = 0;
        fullmove_number_ = 1;
        white_king_ = Square('e', '1');
        black_king_ = Square('e', '8');
    }
};
