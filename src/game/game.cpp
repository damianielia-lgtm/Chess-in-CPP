#include "game.h"

#include <string>
#include <chrono>
#include <optional>
#include <cassert>

#include "../core/position.h"
#include "../core/move.h"
#include "../core/piece.h"
#include "../core/square.h"
#include "../movegen/legal_moves.h"
#include "../movegen/attacks.h"
#include "../notation/uci.h"

namespace {

Square valid_ep_target(const Position& position) {
    Position modifiable_copy = position;

    for (const Move move : all_moves(modifiable_copy, MoveGeneration::All)) {
        if (
            move.is_en_passant() &&
            move.target() == position.en_passant_target()
        ) {
            return move.target();
        }
    }

    return Square();
}

bool is_repeated_position(const Position& position1, const Position& position2) {
    for (const Square square : Square::all()) {
        if (position1.piece_at(square) != position2.piece_at(square)) {
            return false;
        }
    }

    if (position1.turn() != position2.turn()) {
        return false;
    }

    if (position1.castling_rights() != position2.castling_rights()) {
        return false;
    }

    if (valid_ep_target(position1) != valid_ep_target(position2)) {
        return false;
    }

    return true;
}

bool insufficient_material(const Position& position) {
    int minor_count = 0;
    bool has_knight = false;
    std::optional<Color> bishop_square_color;

    for (const Square square : Square::all()) {
        Piece piece = position.piece_at(square);

        if (piece.empty() || piece.type() == PieceType::King) {
            continue;
        }

        switch (piece.type()) {            
            case PieceType::Pawn: return false;
            case PieceType::Rook: return false;
            case PieceType::Queen: return false;

            case PieceType::Knight: has_knight = true; break;

            case PieceType::Bishop: {
                const Color square_color = (square.rank() + square.file()) % 2 == 0
                    ? Color::White
                    : Color::Black;
                if (bishop_square_color && *bishop_square_color != square_color) {
                    return false; // bishops on opposite-colored squares
                }
                bishop_square_color = square_color;
                break;
            }
        }

        if (++minor_count > 2 || (minor_count == 2 && has_knight)) {
            return false; // 3+ minors, or 2 minors including a knight
        }
    }

    return true;
}

}

void GameState::deduct_time(std::chrono::milliseconds time_taken) {
    assert(is_timed_game());

    if (current_position_.turn() == Color::White) {
        clock_->white_time -= time_taken;
        if (clock_->white_time <= std::chrono::milliseconds::zero()) {
            clock_->white_time = std::chrono::milliseconds::zero();
            result_ = GameResult::WhiteTimeout;
        }
    } else {
        clock_->black_time -= time_taken;
        if (clock_->black_time <= std::chrono::milliseconds::zero()) {
            clock_->black_time = std::chrono::milliseconds::zero();
            result_ = GameResult::BlackTimeout;
        }
    }
}

void GameState::play_move(std::string uci_move) {
    assert(!result_.has_value());

    Move move = resolve_uci(current_position_, uci_move);
    moves_history_.push_back(move);

    if (is_timed_game()) {
        if (current_position_.turn() == Color::White) {
            clock_->white_time += clock_->increment;
        } else {
            clock_->black_time += clock_->increment;
        }
    }

    UndoState move_state = current_position_.apply_move(move);
    update_material(move_state.captured_piece);
    positions_history_.push_back(current_position_);
}

void GameState::check_game_end() {
    assert(!result_.has_value());

    if (all_moves(current_position_, MoveGeneration::All).empty()) {
        bool in_check = is_attacked_square(
            current_position_,
            current_position_.king_square(current_position_.turn()),
            current_position_.opposite_turn()
        );
        result_ = (in_check)
            ? ((current_position_.turn() == Color::White)
                ? GameResult::WhiteCheckmated
                : GameResult::BlackCheckmated)
            : GameResult::Stalemate;
    } else if (insufficient_material(current_position_)) {
        result_ = GameResult::InsufficientMaterial;
    } else if (current_position_.halfmove_clock() >= 100) {
        result_ = GameResult::FiftyMoveRule;
    } else {
        int same_positions_count = 0;
        for (const Position& position : positions_history_) {
            if (is_repeated_position(position, current_position_)) { same_positions_count++; }
        }
        if (same_positions_count >= 3) {
            result_ = GameResult::ThreefoldRepetition;
        }
    }
}
