#include "display.h"

#include <iostream>
#include <string_view>

#include "../core/position.h"
#include "../core/square.h"

void print_board(const Position& position, const bool flip) {
    std::string_view files = flip
        ? "    h   g   f   e   d   c   b   a    \n"
        : "    a   b   c   d   e   f   g   h    \n";

    std::cout << files;
    std::cout << "  +---+---+---+---+---+---+---+---+  \n";

    for (int rank = 7; rank >= 0; --rank) {
        int display_rank = flip ? 7 - rank : rank;
        std::cout << display_rank + 1 << " | ";

        for (int file = 0; file <= 7; file++) {
            int display_file = flip ? 7 - file : file;
            std::cout << position.piece_at(Square(display_rank * 8 + display_file)).symbol() << " | ";
        }

        std::cout << display_rank + 1 << '\n';
        std::cout << "  +---+---+---+---+---+---+---+---+  \n";
    }
    
    std::cout << files;
}

void print_pos_info(const Position& position) {
    print_board(position, false);
    std::cout << "Fen: " << position.to_fen() << '\n';
}
