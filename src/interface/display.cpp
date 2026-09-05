#include "display.h"

#include <iostream>
#include <string>
#include <string_view>

#include "../core/position.h"
#include "output_construction.h"

namespace {

constexpr std::string_view help_content =
    "Core\n"
    "   config show                                                                   show current configuration\n"
    "   config set <field> <value>                                                    set configurations\n"
    "   position show                                                                 print current position\n"
    "   position --startpos                                                           set starting position\n"
    "   position --fen \"<fen string>\"                                                 load in specific fen\n"
    "   position --saved-fen \"<saved fen name>\"                                       load in saved fen\n"
    "   move <move>                                                                   apply move to current position\n\n"

    "Game features\n"
    "   play local --untimed                                                          start a local untimed game\n"
    "   play local --time-control <initial+increment>                                 start a local timed game\n"
    "   play online                                                                   start an online game\n"
    "   play engine --player-color {white|black} --depth <n>                          play against the engine\n"
    "   replay <saved pgn name>                                                       replay a saved game\n"
    "   analyze                                                                       analyze current position\n\n"

    "PGN management\n"
    "   pgn list                                                                      list saved PGNs\n"
    "   pgn save <name>                                                               save last game as PGN\n"
    "   pgn show <name>                                                               print a saved PGN\n"
    "   pgn delete <name>                                                             delete a saved PGN\n"
    "   pgn import <file>                                                             load a pgn to game directory\n"
    "   pgn export <name> <directory>                                                 export a saved pgn from game directory\n\n"

    "FEN management\n"
    "   fen list                                                                      list saved FENs\n"
    "   fen save <name>                                                               save current position FEN string under a name\n"
    "   fen show <name>                                                               print a saved FEN\n"
    "   fen delete <name>                                                             delete a saved FEN\n"
    "   fen import <file>                                                             load a fen to game directory\n"
    "   fen export <name> <directory>                                                 export a saved fen from game directory\n\n"

    "Perft testing\n"
    "   perft --preset <instant|fast|moderate|extended>                               test engine corectness through the database\n"
    "   perft --depth <n> [--output <file>]                                           test corectness on a current position.\n"
    "   debug --depth <n> [--output <file>]                                           recusively go through a position and compare with stockfish.\n\n"

    "Benchmarking\n"
    "   benchmark perft --preset <instant|fast|moderate|extended>                     test movegen speed through the database\n"
    "   benchmark perft --depth <n> [--output <file>]                                 test movegen speed on current position\n"
    "   benchmark engine --preset <instant|fast|moderate|extended>                    test engine speed through the database\n"
    "   benchmark engine --depth <n> [--output <file>]                                test engine speed on current position\n\n"

    "Report management\n"
    "   report list                                                                   list saved reports\n"
    "   report save <name>                                                            save latest report under a name\n"
    "   report show <name>                                                            print a saved reports\n"
    "   report delete <name>                                                          delete a saved report\n";

}

void print_help() { std::cout << help_content; }

void print_lines(const std::vector<std::string>& lines) {
    for (const std::string& line : lines) {
        std::cout << line << '\n';
    }
}
