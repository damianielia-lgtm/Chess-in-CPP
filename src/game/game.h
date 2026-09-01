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
    Draw_by_Unknown,

    Unknown_End
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

    bool has_clock_data() const noexcept { return clocks_.has_value(); }
    std::chrono::milliseconds clock(Color color) const noexcept {
        assert(has_clock_data());
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
    LiveGameState(
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

    GameSnapshot make_snapshot() const;

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

struct GameMetadata {
    std::string white_name;
    std::string black_name;
    std::optional<TimeControl> time_control;
    Position starting_position;
};

class Game {
public:
    Game(
        std::string white_name,
        std::string black_name,
        std::optional<TimeControl> time = std::nullopt,
        std::optional<Position> starting_pos = std::nullopt
    ):
        metadata_(GameMetadata{white_name, black_name, time, starting_pos ? *starting_pos : Position()}),
        live_state_(LiveGameState(
            starting_pos ? *starting_pos : Position(),
            std::nullopt,
            {},
            {},
            time
                ? std::optional<ClockState>(ClockState{time->initial, time->initial})
                : std::nullopt
        )),
        snapshots_({live_state_.make_snapshot()}),
        result_(std::nullopt) {}

    const std::vector<GameSnapshot>& all_snapshots() const noexcept { return snapshots_; }
    GameSnapshot live_snapshot() const { return live_state_.make_snapshot(); }
    const Position& live_position() const noexcept { return live_state_.position(); }

    bool has_ended() const noexcept { return result_.has_value(); }
    GameResult result() const noexcept {
        assert(has_ended());
        return *result_;
    }
    
    bool is_timed_game() const noexcept { return metadata_.time_control.has_value(); }
    const std::string& name(Color color) const noexcept {
        return (color == Color::White) ? metadata_.white_name : metadata_.black_name;
    }
    const GameMetadata& metadata() const noexcept { return metadata_; }

    std::size_t snapshot_count() const noexcept { return snapshots_.size(); }
    const GameSnapshot& snapshot_at(std::size_t index) const noexcept {
        assert(index < snapshots_.size());
        return snapshots_[index];
    }
    void pop_state() noexcept {
        assert(!snapshots_.empty());
        snapshots_.pop_back();
        live_state_ = LiveGameState(
            snapshots_.back().position(),
            snapshots_.back().last_move(),
            snapshots_.back().captures(Color::White),
            snapshots_.back().captures(Color::Black),
            snapshots_.back().has_clock_data()
                ? std::optional<ClockState>(ClockState{snapshots_.back().clock(Color::White), snapshots_.back().clock(Color::Black)})
                : std::nullopt
        );
        result_ = std::nullopt;
        check_game_end();
    }
    void finish_without_result() noexcept { assert(!result_.has_value()); result_ = GameResult::Unknown_End; }

    void consume_time(std::chrono::milliseconds time_taken) noexcept;
    void play_move(Move move);

    void resign() noexcept;
    void agree_draw() noexcept;
    void check_game_end();
    void record_unknown_result(GameResult result) noexcept;

private:
    GameMetadata metadata_;

    LiveGameState live_state_;
    std::vector<GameSnapshot> snapshots_;

    std::optional<GameResult> result_;
    void finish(GameResult result) noexcept { assert(!result_.has_value()); result_ = result; }
};
