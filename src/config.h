#pragma once

#include <string>

enum class BoardOrientation { White, Black };
enum class MoveInput { Uci, San };

struct ConfigData {
    std::string white_name = "White";
    std::string black_name = "Black";

    std::string event = "?";
    std::string site = "Elia.chess";
    bool pgn_save_clock = true;

    MoveInput move_input = MoveInput::Uci;
    BoardOrientation board_orientation = BoardOrientation::White;
};
