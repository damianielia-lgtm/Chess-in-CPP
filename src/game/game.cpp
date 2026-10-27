#include "game.h"

#include <algorithm>
#include <utility>

#include "../movegen/attacks.h"
#include "../movegen/legal_moves.h"

namespace {

Square valid_ep_target(const Position& position) {
    Position copy = position;
    for (const Move move : all_moves(copy)) {
        if (move.is_en_passant() && move.target() == position.en_passant_target()) {
            return move.target();
        }
    }
    return Square();
}

bool is_repeated_position(const Position& first, const Position& second) {
    for (const Square square : Square::all()) {
        if (first.piece_at(square) != second.piece_at(square)) {
            return false;
        }
    }
    return first.turn() == second.turn()
        && first.castling_rights() == second.castling_rights()
        && valid_ep_target(first) == valid_ep_target(second);
}

bool insufficient_material(const Position& position) {
    int minor_count = 0;
    bool has_knight = false;
    std::optional<int> bishop_square_color;

    for (const Square square : Square::all()) {
        const Piece piece = position.piece_at(square);
        switch (piece.type()) {
            case PieceType::Empty:
            case PieceType::King:
                continue;
            case PieceType::Knight:
                has_knight = true;
                break;
            case PieceType::Bishop: {
                const int color = (square.rank() + square.file()) % 2;
                if (bishop_square_color && *bishop_square_color != color) {
                    return false;
                }
                bishop_square_color = color;
                break;
            }
            default:
                return false;
        }

        // At most two bishops on the same color, or a single minor piece.
        if (++minor_count > 2 || (minor_count == 2 && has_knight)) {
            return false;
        }
    }
    return true;
}

}

GameSnapshot::GameSnapshot(
    Position position,
    std::optional<Move> move,
    std::vector<Piece> white_captures,
    std::vector<Piece> black_captures,
    std::optional<ClockState> clock
) :
    position_(std::move(position)),
    last_move_(move),
    white_captures_(std::move(white_captures)),
    black_captures_(std::move(black_captures)),
    clocks_(clock) {}

int GameSnapshot::material_comparison() const noexcept {
    int material = 0;
    for (const Square square : Square::all()) {
        material += position_.piece_at(square).material_value();
    }
    return material;
}

LiveGameState::LiveGameState(
    Position position,
    std::optional<Move> move,
    std::vector<Piece> white_captures,
    std::vector<Piece> black_captures,
    std::optional<ClockState> clock
) :
    snapshot_(std::move(position), move, std::move(white_captures), std::move(black_captures), clock) {}

std::chrono::milliseconds& LiveGameState::remaining_time(Color side) noexcept {
    assert(is_timed_game());
    return side == Color::White ? snapshot_.clocks_->white_time : snapshot_.clocks_->black_time;
}

void LiveGameState::consume_time(Color side, std::chrono::milliseconds time_taken) noexcept {
    auto& time = remaining_time(side);
    time = std::max(time - time_taken, std::chrono::milliseconds::zero());
}

void LiveGameState::add_increment(Color side, std::chrono::milliseconds increment) noexcept {
    remaining_time(side) += increment;
}

void LiveGameState::play_move(Move move) {
    snapshot_.last_move_ = move;
    update_captures(snapshot_.position_.apply_move(move).captured_piece);
}

void LiveGameState::update_captures(Piece captured_piece) noexcept {
    if (captured_piece.empty()) { return; }
    auto& captures = captured_piece.color() == Color::White
        ? snapshot_.black_captures_
        : snapshot_.white_captures_;
    captures.push_back(captured_piece);
}

Game::Game(
    std::string white_name,
    std::string black_name,
    std::string event,
    std::string site,
    std::optional<TimeControl> time,
    std::optional<Position> starting_pos,
    std::optional<int> opening_id,
    std::optional<std::string> eco_tag
) :
    metadata_{
        std::move(white_name),
        std::move(black_name),
        std::move(event),
        std::move(site),
        time,
        starting_pos.value_or(Position()),
        opening_id,
        eco_tag
    },
    live_state_(
        metadata_.starting_position, std::nullopt, {}, {},
        time ? std::optional<ClockState>{{time->initial, time->initial}} : std::nullopt
    ),
    snapshots_{live_state_.make_snapshot()} {}

void Game::pop_state() noexcept {
    assert(snapshots_.size() > 1);
    snapshots_.pop_back();
    live_state_ = LiveGameState(snapshots_.back());
    result_.reset();
    check_game_end();
}

void Game::consume_time(std::chrono::milliseconds time_taken) noexcept {
    assert(!has_ended());
    assert(is_timed_game());
    live_state_.consume_time(live_state_.turn(), time_taken);
}

void Game::play_move(Move move) {
    assert(!has_ended());
    const Color mover = live_state_.turn();
    live_state_.play_move(move);
    if (is_timed_game()) {
        live_state_.add_increment(mover, metadata_.time_control->increment);
    }
    snapshots_.push_back(live_state_.make_snapshot());
}

void Game::resign() noexcept {
    finish(live_state_.turn() == Color::White
        ? GameResult::Black_by_Resignation
        : GameResult::White_by_Resignation);
}

void Game::agree_draw() noexcept {
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
    if (all_moves(position).empty()) {
        const bool in_check = is_attacked_square(
            position, position.king_square(position.turn()), position.opposite_turn()
        );
        finish(in_check
            ? (position.turn() == Color::White
                ? GameResult::Black_by_Checkmate : GameResult::White_by_Checkmate)
            : GameResult::Draw_by_Stalemate);
    } else if (insufficient_material(position)) {
        finish(GameResult::Draw_by_InsufficientMaterial);
    } else if (position.halfmove_clock() >= 100) {
        finish(GameResult::Draw_by_FiftyMove);
    } else {
        const auto occurrences = std::count_if(snapshots_.begin(), snapshots_.end(),
            [&](const GameSnapshot& snapshot) {
                return is_repeated_position(position, snapshot.position());
            });
        if (occurrences >= 3) {
            finish(GameResult::Draw_by_ThreefoldRepetition);
        }
    }
}
