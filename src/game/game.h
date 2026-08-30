#pragma once

#include <vector>
#include <string>
#include <optional>
#include <chrono>
#include <cassert>
#include <algorithm>
#include <utility>

#include "../core/position.h"
#include "../core/piece.h"
#include "../core/move.h"

enum class GameResult {
    White_by_Checkmate,
    White_by_Resignation,
    White_by_Timeout,
    White_by_Unknown,

    Black_by_Checkmate,
    Black_by_Resignation,
    Black_by_Timeout,
    Black_by_Unknown,

    Draw_by_Stalemate,
    Draw_by_InsufficientMaterial,
    Draw_by_FiftyMove,
    Draw_by_ThreefoldRepetition,
    Draw_by_Agreement,
    Draw_by_Unknown
};

struct TimeControl {
    std::chrono::milliseconds initial;
    std::chrono::milliseconds increment;
};

struct ClockState {
    std::chrono::milliseconds white_time;
    std::chrono::milliseconds black_time;
};

class GameSnapshot {
public:
    GameSnapshot(
        Position position,
        std::optional<Move> move,
        std::vector<Piece> white_captures,
        std::vector<Piece> black_captures,
        std::optional<ClockState> clock
    ):
        position_(std::move(position)),
        incoming_move_(std::move(move)),
        white_captured_material_(std::move(white_captures)),
        black_captured_material_(std::move(black_captures)),
        clocks_(std::move(clock)) {}

    const Position& position() const noexcept { return position_; }
    Color turn() const noexcept { return position_.turn(); }

    std::optional<Move> last_move() const noexcept { return incoming_move_; }

    const std::vector<Piece>& captures(Color color) const noexcept {
        return (color == Color::White
            ? white_captured_material_
            : black_captured_material_);
    }
    int material_comparison() const noexcept {
        int material_comparison = 0;
        for (const Square square : Square::all()) {
            material_comparison += position_.piece_at(square).material_value();
        }
        return material_comparison;
    }

    bool is_timed_game() const noexcept { return clocks_.has_value(); }
    std::chrono::milliseconds clock(Color color) const noexcept {
        assert(is_timed_game());
        return (color == Color::White
            ? clocks_->white_time
            : clocks_->black_time);
    }

private:
    Position position_;
    std::optional<Move> incoming_move_;
    std::vector<Piece> white_captured_material_;
    std::vector<Piece> black_captured_material_;
    std::optional<ClockState> clocks_;
};

class LiveGameState {
public:
    LiveGameState(std::optional<std::chrono::milliseconds> initial = std::nullopt):
        position_(Position()),
        incoming_move_(std::nullopt),
        white_captured_material_({}),
        black_captured_material_({}),
        clocks_(initial
            ? std::optional<ClockState>(ClockState{*initial, *initial})
            : std::nullopt) {}

    GameSnapshot make_snapshot() const {
        return GameSnapshot(
            position_,
            incoming_move_,
            white_captured_material_,
            black_captured_material_,
            clocks_
        );
    }

    const Position& position() const noexcept { return position_; }
    Color turn() const noexcept { return position_.turn(); }
    bool is_timed_game() const noexcept { return clocks_.has_value(); }
    std::chrono::milliseconds clock(Color color) const noexcept {
        assert(is_timed_game());
        return (color == Color::White
            ? clocks_->white_time
            : clocks_->black_time);
    }

    void consume_time(Color side, std::chrono::milliseconds time_taken) noexcept;
    void add_increment(Color side, std::chrono::milliseconds increment) noexcept;
    void play_move(Move move);

private:
    Position position_;
    std::optional<Move> incoming_move_;
    std::vector<Piece> white_captured_material_;
    std::vector<Piece> black_captured_material_;
    std::optional<ClockState> clocks_;

    void update_captures(const Piece captured_piece) noexcept;
};

class Game {
public:
    Game(
        std::string white_name,
        std::string black_name,
        std::optional<TimeControl> time = std::nullopt
    ):
        white_name_(std::move(white_name)),
        black_name_(std::move(black_name)),
        time_control_(time),
        live_state_(LiveGameState(time
            ? std::optional<std::chrono::milliseconds>{time->initial}
            : std::optional<std::chrono::milliseconds>{})),
        result_(std::nullopt),
        snapshots_({live_state_.make_snapshot()}) {}

    const std::vector<GameSnapshot>& all_snapshots() const noexcept { return snapshots_; }
    GameSnapshot live_snapshot() const { return live_state_.make_snapshot(); }
    const Position& live_position() const noexcept { return live_state_.position(); }

    bool has_ended() const noexcept { return result_.has_value(); }
    GameResult result() const noexcept {
        assert(has_ended());
        return *result_;
    }
    
    bool is_timed_game() const noexcept { return time_control_.has_value(); }

    const std::string& name(Color color) const noexcept {
        return (color == Color::White) ? white_name_ : black_name_;
    }

    std::size_t snapshot_count() const noexcept { return snapshots_.size(); }
    const GameSnapshot& snapshot_at(std::size_t index) const noexcept {
        assert(index < snapshots_.size());
        return snapshots_[index];
    }

    void consume_time(std::chrono::milliseconds time_taken) noexcept;
    void play_move(Move move);

    void resign() noexcept;
    void agree_draw() noexcept;
    void check_game_end();
    void record_unknown_result(GameResult result) noexcept;

private:
    std::string white_name_;
    std::string black_name_;
    std::optional<TimeControl> time_control_;

    LiveGameState live_state_;
    std::optional<GameResult> result_;
    void finish(GameResult result) noexcept { assert(!result_.has_value()); result_ = result; }
    std::vector<GameSnapshot> snapshots_;
};
