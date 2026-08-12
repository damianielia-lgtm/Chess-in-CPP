#pragma once

#include <vector>
#include <string>
#include <string_view>
#include <optional>
#include <chrono>

#include "../core/position.h"
#include "../core/piece.h"
#include "../movegen/attacks.h"

enum class GameResult {
    WhiteCheckmate,
    BlackCheckmate,
    WhiteResign,
    BlackResign,
    WhiteTimeout,
    BlackTimeout,

    Stalemate,
    InsufficientMaterial,
    FiftyMoveRule,
    ThreefoldRepetition,
    Agreement
};

struct TimeControl {
    std::chrono::milliseconds initial;
    std::chrono::milliseconds increment;
};

struct ClockState {
    std::chrono::milliseconds white_time;
    std::chrono::milliseconds black_time;
    std::chrono::milliseconds increment;
};

class GameState {
public:
    GameState(std::optional<TimeControl> time = std::nullopt):
        current_position_(Position()),
        result_(std::nullopt),
        positions_history_({Position()}),
        uci_moves_history_({}),
        white_captured_material_({}),
        black_captured_material_({}),
        material_comparison_(0),
        clock_(time
            ? std::optional<ClockState>(ClockState{time->initial, time->initial, time->increment})
            : std::nullopt) {}

    void play_move(std::string uci_move, std::optional<std::chrono::milliseconds> time_taken);

    const Position& current_position() const noexcept { return current_position_; }

    std::string_view turn_name() const noexcept {
        return (current_position_.turn() == Color::White ? "White" : "Black");
    }

    bool has_ended() const noexcept { return result_.has_value(); }
    GameResult result() const noexcept {
        assert(has_ended());
        return result_.value();
    }

    const std::vector<Piece>& captures(Color color) const noexcept {
        return (color == Color::White
            ? white_captured_material_
            : black_captured_material_);
    }
    const int material_comparison() const noexcept { return material_comparison_; }

    const bool is_timed_game() const noexcept { return clock_.has_value(); }
    const std::chrono::milliseconds current_clock(Color color) const noexcept {
        assert(is_timed_game());
        return (color == Color::White
            ? clock_->white_time
            : clock_->black_time);
    }

private:
    Position current_position_;
    std::optional<GameResult> result_;

    std::vector<Position> positions_history_;
    std::vector<std::string> uci_moves_history_;

    std::vector<Piece> white_captured_material_;
    std::vector<Piece> black_captured_material_;
    int material_comparison_;

    void update_material(const Piece captured_piece) noexcept {
        if (!captured_piece.empty()) {
            if (captured_piece.color() == Color::White) {
                black_captured_material_.push_back(captured_piece);
            } else if (captured_piece.color() == Color::Black) {
                white_captured_material_.push_back(captured_piece);
            }
        }

        material_comparison_ = 0;
        for (const Square square : Square::all()) {
            material_comparison_ += current_position_.piece_at(square).material_value();
        }
    }

    std::optional<ClockState> clock_;
};
