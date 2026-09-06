#pragma once

#include <string>
#include <cstdint>

#include "notation/move_notation.h"

enum class BoardOrientation { White, Black };

struct ConfigData {
    std::string player1_name = "White";
    std::string player2_name = "Black";

    std::string event = "?";
    std::string site = "Elia.chess";
    bool pgn_save_clock = true;

    MoveNotation move_notation = MoveNotation::Uci;
    BoardOrientation board_orientation = BoardOrientation::White;

    std::uint8_t engine_depth = 5;
};
