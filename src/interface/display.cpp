#include "display.h"

#include <iostream>
#include <string>

#include "../core/position.h"
#include "output_construction.h"

void print_pos_info(const Position& position) {
    for (const std::string& line : construct_board_lines(position, false)) {
        std::cout << line << '\n';
    }
    std::cout << "Fen: " << position.to_fen() << '\n';
}
