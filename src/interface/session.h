#pragma once

#include <string>
#include <string_view>
#include <optional>
#include <utility>

#include "../core/position.h"
#include "../core/move.h"
#include "../notation/uci.h"
#include "../game/game.h"
#include "../errors.h"

class Session {
public:
    Session() = default;

    void reset_pos() noexcept { current_position_ = Position(); }
    void set_pos(const std::string_view fen_string) { current_position_ = Position(fen_string); }
    void apply_uci_move(const std::string_view uci_move) {
        current_position_.apply_move(resolve_uci(current_position_, uci_move));
    }

    const Position& current_position() const noexcept { return current_position_; }

    void store_last_game(GameState game) { last_game_ = std::move(game); }
    const GameState& last_game() const {
        if (!last_game_) {
            throw SessionError("No last game exists");
        }

        return last_game_.value();
    }

private:
    Position current_position_;
    std::optional<GameState> last_game_;
};
