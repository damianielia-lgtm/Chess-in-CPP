#pragma once

#include <string>
#include <string_view>

#include "../core/position.h"
#include "../core/move.h"

class Session {
public:
    Session() = default;

    void reset_pos() noexcept { current_position_ = Position(); }
    void set_pos(const std::string_view fen_string) { current_position_ = Position(fen_string); }
    void apply_uci_move(const std::string_view uci_move) {
        current_position_.apply_move(current_position_.resolve_uci(uci_move));
    }

    [[nodiscard]] const Position& current_position() const noexcept { return current_position_; }
    [[nodiscard]] const std::string fen() const noexcept { return current_position_.to_fen(); }

private:
    Position current_position_;
};
