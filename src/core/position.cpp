#include "position.h"

#include "square.h"
#include "piece.h"
#include "move.h"
#include "castling_rights.h"

void Position::make_move(const Move move) noexcept {
    Square origin = move.origin();
    Square target = move.target();

    if (move.is_promotion()) {
        set_piece(target, Piece(turn_, move.promotion_type()));
    } else {
        set_piece(target, piece_at(origin));
    }

    set_piece(origin, Piece());

    if (move.is_en_passant()) {
        set_piece(move.en_passant_capture(), Piece());
    }

    if (move.is_castling()) {
        Move rook_movement = CastlingOption(move).castling_rook_movement();
        set_piece(rook_movement.origin(), Piece());
        set_piece(rook_movement.target(), Piece(turn_, PieceType::Rook));
    }
}

void Position::unmake_move(const Move move, const Piece capture) noexcept {
    Square origin = move.origin();
    Square target = move.target();

    if (move.is_promotion()) {
        set_piece(origin, Piece(turn_, PieceType::Pawn));
    } else {
        set_piece(origin, piece_at(target));
    }

    if (move.is_en_passant()) {
        set_piece(move.en_passant_capture(), capture);
        set_piece(target, Piece());
    } else {
        set_piece(target, capture);
    }

    if (move.is_castling()) {
        Move rook_movement = CastlingOption(move).castling_rook_movement();
        set_piece(rook_movement.target(), Piece());
        set_piece(rook_movement.origin(), Piece(turn_, PieceType::Rook));
    }
}

UndoState Position::apply_move(const Move move) noexcept {
    Piece moving_piece = piece_at(move.origin());
    Piece captured_piece = piece_at(move.target());

    UndoState undo_state{};
    undo_state.captured_piece = (move.is_en_passant()) ? piece_at(move.en_passant_capture()) : piece_at(move.target());
    undo_state.castling_rights = castling_rights_;
    undo_state.en_passant_target = en_passant_target_;
    undo_state.halfmove_clock = halfmove_clock_;
    undo_state.fullmove_number = fullmove_number_;
    undo_state.white_king = white_king_;
    undo_state.black_king = black_king_;
    
    make_move(move);
    
    turn_ = opposite_turn();

    if (moving_piece.type() == PieceType::King) {
        castling_rights_.revoke(CastlingOption(moving_piece.color(), CastlingSide::Kingside));
        castling_rights_.revoke(CastlingOption(moving_piece.color(), CastlingSide::Queenside));
    }
    if (moving_piece.type() == PieceType::Rook) {
        castling_rights_.revoke_from_rook(move.origin());
    }
    if (captured_piece.type() == PieceType::Rook) {
        castling_rights_.revoke_from_rook(move.target());
    }

    en_passant_target_ = (move.is_double_pawn()) ? Square((move.origin().index() + move.target().index()) / 2) : Square();

    if (moving_piece.type() == PieceType::Pawn || move.is_capture()) {
        halfmove_clock_ = 0;
    } else {
        halfmove_clock_++;
    }

    if (turn_ == Color::White) { fullmove_number_++; }

    if (moving_piece == Piece(Color::White, PieceType::King)) {
        white_king_ = move.target();
    } else if (moving_piece == Piece(Color::Black, PieceType::King)) {
        black_king_ = move.target();
    }

    return undo_state;
}

void Position::revert_move(const Move move, const UndoState& undo_state) noexcept {
    turn_ = opposite_turn();
    unmake_move(move, undo_state.captured_piece);
    castling_rights_ = undo_state.castling_rights;
    en_passant_target_ = undo_state.en_passant_target;
    halfmove_clock_ = undo_state.halfmove_clock;
    fullmove_number_ = undo_state.fullmove_number;
    white_king_ = undo_state.white_king;
    black_king_ = undo_state.black_king;
}
