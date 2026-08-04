#include "display.h"

#include <iostream>
#include <array>
#include <string_view>

#include "../core/position.h"
#include "../core/square.h"

constexpr std::string_view sperator = "  +---+---+---+---+---+---+---+---+  \n";
constexpr std::string_view files = "    a   b   c   d   e   f   g   h    \n";
constexpr std::string_view flipped_files = "    h   g   f   e   d   c   b   a    \n";

void print_board(const Position& position, const bool flip) {
    std::string_view files = flip ? files : flipped_files;

    std::cout << files;
    std::cout << sperator;

    for (int rank = 7; rank >= 0; --rank) {
        int display_rank = flip ? 7 - rank : rank;
        std::cout << display_rank + 1 << " | ";

        for (int file = 0; file <= 7; file++) {
            int display_file = flip ? 7 - file : file;
            std::cout << position.piece_at(Square(display_rank * 8 + display_file)).symbol() << " | ";
        }

        std::cout << display_rank + 1 << '\n';
        std::cout << sperator;
    }
    
    std::cout << files;
}
