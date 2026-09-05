#pragma once

#include <string>
#include <vector>
#include <string_view>
#include <optional>
#include <utility>

#include "../core/position.h"
#include "../core/move.h"
#include "../game/game.h"
#include "../errors.h"

class Session {
public:
    Session() = default;

    void reset_pos() noexcept { current_position_ = Position(); }
    void set_pos(std::string_view fen_string) { current_position_ = Position(fen_string); }
    void apply_move(Move move) { current_position_.apply_move(move); }

    const Position& current_position() const noexcept { return current_position_; }

    void store_last_game(Game game) { last_game_ = std::move(game); }
    const Game& last_game() const {
        if (!last_game_) {
            throw SessionError("No last game exists");
        }

        return *last_game_;
    }
    
    void store_last_report(std::vector<std::string> lines) { last_report_ = std::move(lines); }
    const std::vector<std::string>& last_report() const {
        if (!last_report_) {
            throw SessionError("No last report exists");
        }

        return *last_report_;
    }

private:
    Position current_position_;
    std::optional<Game> last_game_;
    std::optional<std::vector<std::string>> last_report_;
};
