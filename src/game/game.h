#pragma once

#include <cassert>
#include <chrono>
#include <cstddef>
#include <optional>
#include <string>
#include <vector>

#include "../core/move.h"
#include "../core/piece.h"
#include "../core/position.h"

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
    );

    const Position& position() const noexcept { return position_; }
    Color turn() const noexcept { return position_.turn(); }
    std::optional<Move> last_move() const noexcept { return last_move_; }
    const std::vector<Piece>& captures(Color color) const noexcept {
        return color == Color::White ? white_captures_ : black_captures_;
    }
    int material_comparison() const noexcept;

    bool has_clock_data() const noexcept { return clocks_.has_value(); }
    std::chrono::milliseconds clock(Color color) const noexcept {
        assert(has_clock_data());
        return color == Color::White ? clocks_->white_time : clocks_->black_time;
    }

private:
    friend class LiveGameState;

    Position position_;
    std::optional<Move> last_move_;
    std::vector<Piece> white_captures_;
    std::vector<Piece> black_captures_;
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
    );

    GameSnapshot make_snapshot() const { return snapshot_; }
    const Position& position() const noexcept { return snapshot_.position(); }
    Color turn() const noexcept { return snapshot_.turn(); }
    bool is_timed_game() const noexcept { return snapshot_.has_clock_data(); }
    std::chrono::milliseconds clock(Color color) const noexcept { return snapshot_.clock(color); }

    void consume_time(Color side, std::chrono::milliseconds time_taken) noexcept;
    void add_increment(Color side, std::chrono::milliseconds increment) noexcept;
    void play_move(Move move);

private:
    friend class Game;

    explicit LiveGameState(const GameSnapshot& snapshot) : snapshot_(snapshot) {}
    std::chrono::milliseconds& remaining_time(Color side) noexcept;
    void update_captures(Piece captured_piece) noexcept;

    GameSnapshot snapshot_;
};

struct GameMetadata {
    std::string white_name;
    std::string black_name;
    std::string event;
    std::string site;
    std::optional<TimeControl> time_control;
    Position starting_position;
};

class Game {
public:
    Game(
        std::string white_name,
        std::string black_name,
        std::string event,
        std::string site,
        std::optional<TimeControl> time = std::nullopt,
        std::optional<Position> starting_pos = std::nullopt
    );

    const std::vector<GameSnapshot>& all_snapshots() const noexcept { return snapshots_; }
    GameSnapshot live_snapshot() const { return live_state_.make_snapshot(); }
    const Position& live_position() const noexcept { return live_state_.position(); }
    std::size_t snapshot_count() const noexcept { return snapshots_.size(); }
    const GameSnapshot& snapshot_at(std::size_t index) const noexcept {
        assert(index < snapshots_.size());
        return snapshots_[index];
    }

    bool has_ended() const noexcept { return result_.has_value(); }
    GameResult result() const noexcept {
        assert(has_ended());
        return *result_;
    }
    bool is_timed_game() const noexcept { return metadata_.time_control.has_value(); }
    const std::string& name(Color color) const noexcept {
        return color == Color::White ? metadata_.white_name : metadata_.black_name;
    }
    const GameMetadata& metadata() const noexcept { return metadata_; }

    void pop_state() noexcept;
    void finish_without_result() noexcept { finish(GameResult::Unknown_End); }
    void consume_time(std::chrono::milliseconds time_taken) noexcept;
    void play_move(Move move);
    void resign() noexcept;
    void agree_draw() noexcept;
    void check_game_end();
    void record_unknown_result(GameResult result) noexcept;

private:
    void finish(GameResult result) noexcept {
        assert(!has_ended());
        result_ = result;
    }

    GameMetadata metadata_;
    LiveGameState live_state_;
    std::vector<GameSnapshot> snapshots_;
    std::optional<GameResult> result_;
};
