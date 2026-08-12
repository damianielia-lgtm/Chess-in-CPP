#include "game.h"

#include <string>
#include <iostream>
#include <chrono>
#include <optional>
#include <stdexcept>

#include "../core/position.h"
#include "../core/move.h"
#include "../core/piece.h"
#include "../core/square.h"
#include "../movegen/legal_moves.h"
#include "../movegen/attacks.h"
#include "../interface/display.h"

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

void GameState::play_move(
    std::string uci_move,
    std::optional<std::chrono::milliseconds> time_taken
) {
    if (result_) {
        throw std::runtime_error("Game has already ended");
    }

    Color move_submitter = current_position_.turn();

    if (is_timed_game()) {
        assert(time_taken);
        if (move_submitter == Color::White) {
            clock_->white_time -= time_taken.value();
            if (clock_->white_time <= std::chrono::milliseconds::zero()) {
                result_ = GameResult::WhiteTimeout;
                return;
            }
        } else {
            clock_->black_time -= time_taken.value();
            if (clock_->black_time <= std::chrono::milliseconds::zero()) {
                result_ = GameResult::BlackTimeout;
                return;
            }
        }
    }

    if (uci_move == "resign") {
        result_ = (move_submitter == Color::White)
            ? GameResult::WhiteResign
            : GameResult::BlackResign;
        return;
    }

    if (uci_move == "draw") {
        result_ = GameResult::Agreement;
        return;
    }

    Move move = current_position_.resolve_uci(uci_move);
    UndoState move_state = current_position_.apply_move(move);

    update_material(move_state.captured_piece);

    positions_history_.push_back(current_position_);
    uci_moves_history_.push_back(uci_move);

    if (is_timed_game()) {
        if (move_submitter == Color::White) {
            clock_->white_time += clock_->increment;
        } else {
            clock_->black_time += clock_->increment;
        }
    }

    if (all_moves(current_position_, false).empty()) {
        bool in_check = is_attacked_square(
            current_position_,
            current_position_.king_square(current_position_.turn()),
            current_position_.opposite_turn()
        );
        result_ = (in_check)
            ? ((current_position_.turn() == Color::White)
                ? GameResult::BlackCheckmate
                : GameResult::WhiteCheckmate)
            : GameResult::Stalemate;
    } else if (insufficient_material(current_position_)) {
        result_ = GameResult::InsufficientMaterial;
    } else if (current_position_.halfmove_clock() >= 100) {
        result_ = GameResult::FiftyMoveRule;
    } else {
        int same_positions_count = 0;
        for (const Position position : positions_history_) {
            if (position == current_position_) { same_positions_count++; }
        }
        if (same_positions_count >= 3) {
            result_ = GameResult::ThreefoldRepetition;
        }
    }
}
