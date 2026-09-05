#include "game.h"

#include <chrono>
#include <optional>
#include <cassert>
#include <algorithm>

#include "../core/position.h"
#include "../core/move.h"
#include "../core/piece.h"
#include "../core/square.h"
#include "../movegen/legal_moves.h"
#include "../movegen/attacks.h"

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

void Game::check_game_end() {
    assert(!has_ended());

    if (is_timed_game()) {
        if (live_state_.clock(Color::White) <= std::chrono::milliseconds::zero()) {
            finish(GameResult::Black_by_Timeout);
            return;
        }
        if (live_state_.clock(Color::Black) <= std::chrono::milliseconds::zero()) {
            finish(GameResult::White_by_Timeout);
            return;
        }
    }

    Position position = live_state_.position();
    if (all_moves(position, MoveGeneration::All).empty()) {
        bool in_check = is_attacked_square(
            position,
            position.king_square(position.turn()),
            position.opposite_turn()
        );
        finish(
            in_check
            ? ((live_state_.turn() == Color::White)
                ? GameResult::Black_by_Checkmate
                : GameResult::White_by_Checkmate)
            : GameResult::Draw_by_Stalemate
        );
    }
    
    else if (insufficient_material(position)) {
        finish(GameResult::Draw_by_InsufficientMaterial);
    }
    
    else if (position.halfmove_clock() >= 100) {
        finish(GameResult::Draw_by_FiftyMove);
    }
    
    else {
        int same_positions_count = 0;
        for (const GameSnapshot& snapshot : snapshots_) {
            if (is_repeated_position(position, snapshot.position())) { same_positions_count++; }
        }
        if (same_positions_count >= 3) {
            finish(GameResult::Draw_by_ThreefoldRepetition);
        }
    }
}

void Game::consume_time(std::chrono::milliseconds time_taken) noexcept {
    assert(!has_ended());
    assert(is_timed_game());
    live_state_.consume_time(live_state_.turn(), time_taken);
}

void Game::play_move(Move move) {
    assert(!has_ended());

    Color mover = live_state_.turn();

    live_state_.play_move(move);

    if (is_timed_game()) {
        live_state_.add_increment(mover, metadata_.time_control->increment);
    }

    snapshots_.push_back(live_state_.make_snapshot());
}

void Game::resign() noexcept {
    assert(!has_ended());
    finish(
        live_position().turn() == Color::White
            ? GameResult::Black_by_Resignation
            : GameResult::White_by_Resignation
    );
}

void Game::agree_draw() noexcept {
    assert(!has_ended());
    finish(GameResult::Draw_by_Agreement);
}

void Game::record_unknown_result(GameResult result) noexcept {
    assert(
        result == GameResult::White_by_Unknown ||
        result == GameResult::Black_by_Unknown ||
        result == GameResult::Draw_by_Unknown ||
        result == GameResult::Unknown_End
    );

    finish(result);
}

void LiveGameState::consume_time(Color side, std::chrono::milliseconds time_taken) noexcept {
    assert(is_timed_game());

    if (side == Color::White) {
        clocks_->white_time -= time_taken;
        clocks_->white_time = std::max(clocks_->white_time, std::chrono::milliseconds::zero());
    } else {
        clocks_->black_time -= time_taken;
        clocks_->black_time = std::max(clocks_->black_time, std::chrono::milliseconds::zero());
    }
}

void LiveGameState::add_increment(Color side, std::chrono::milliseconds increment) noexcept {
    assert(is_timed_game());

    if (side == Color::White) {
        clocks_->white_time += increment;
    } else {
        clocks_->black_time += increment;
    }
}

void LiveGameState::play_move(Move move) {
    incoming_move_ = move;
    UndoState move_state = position_.apply_move(move);
    update_captures(move_state.captured_piece);
}

void LiveGameState::update_captures(const Piece captured_piece) noexcept {
    if (!captured_piece.empty()) {
        if (captured_piece.color() == Color::White) {
            black_captured_material_.push_back(captured_piece);
        } else if (captured_piece.color() == Color::Black) {
            white_captured_material_.push_back(captured_piece);
        }
    }
}

GameSnapshot LiveGameState::make_snapshot() const {
    return GameSnapshot(
        position_,
        incoming_move_,
        white_captured_material_,
        black_captured_material_,
        clocks_
    );
}
