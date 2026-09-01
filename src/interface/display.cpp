#include "display.h"

#include <iostream>
#include <string>
#include <string_view>

#include "../core/position.h"
#include "output_construction.h"

namespace {

constexpr std::string_view help_content =
    "\033[1mCore\033[0m\n"
    "   \033[90mconfig\033[0m                                                                        set resource paths, player names\n"
    "   \033[90mposition show\033[0m                                                                 print current position\n"
    "   \033[90mposition --startpos\033[0m                                                           set starting position\n"
    "   \033[90mposition --fen \"<fen string>\"\033[0m                                                 load in specific fen\n"
    "   \033[90mmove <uci move>\033[0m                                                               apply uci move to current position\n\n"

    "\033[1mGame features\033[0m\n"
    "   \033[90mplay local [--time-control <initial+increment>]\033[0m                               start a local game, both sides on this machine\n"
    "   \033[90mplay online\033[0m                                                                   start an online game\n"
    "   \033[90mplay engine --player-color {white|black} --depth <n>\033[0m                          play against the engine\n"
    "   \033[90mreplay <saved pgn name>\033[0m                                                       replay a saved game\n"
    "   \033[90manalyze\033[0m                                                                       analyze current position\n\n"

    "\033[1mPGN management\033[0m\n"
    "   \033[90mpgn list\033[0m                                                                      list saved PGNs\n"
    "   \033[90mpgn save <name>\033[0m                                                               save last game as PGN\n"
    "   \033[90mpgn show <name>\033[0m                                                               print a saved PGN\n"
    "   \033[90mpgn delete <name>\033[0m                                                             delete a saved PGN\n"
    "   \033[90mpgn import <file> --name <name>\033[0m                                               load a pgn to game directory\n"
    "   \033[90mpgn export <name> --output <file>\033[0m                                             export a saved pgn from game directory\n\n"

    "\033[1mFEN management\033[0m\n"
    "   \033[90mfen list\033[0m                                                                      list saved FENs\n"
    "   \033[90mfen save <name>\033[0m                                                               save current position FEN string under a name\n"
    "   \033[90mfen show <name>\033[0m                                                               print a saved FEN\n"
    "   \033[90mfen delete <name>\033[0m                                                             delete a saved FEN\n"
    "   \033[90mfen import <file> --name <name>\033[0m                                               load a fen to game directory\n"
    "   \033[90mfen export <name> --output <file>\033[0m                                             export a saved fen from game directory\n\n"

    "\033[1mPerft testing\033[0m\n"
    "   \033[90mperft --preset <instant|fast|moderate|extended>\033[0m                               test engine corectness through the database\n"
    "   \033[90mperft --depth <n> [--output <file>]\033[0m                                           test corectness on a current position.\n"
    "   \033[90mdebug --depth <n> [--output <file>]\033[0m                                           recusively go through a position and compare with stockfish.\n\n"

    "\033[1mBenchmarking\033[0m\n"
    "   \033[90mbenchmark perft --preset <instant|fast|moderate|extended>\033[0m                     test movegen speed through the database\n"
    "   \033[90mbenchmark perft --depth <n> [--output <file>]\033[0m                                 test movegen speed on current position\n"
    "   \033[90mbenchmark engine --preset <instant|fast|moderate|extended>\033[0m                    test engine speed through the database\n"
    "   \033[90mbenchmark engine --depth <n> [--output <file>]\033[0m                                test engine speed on current position\n\n"

    "\033[1mReport management\033[0m\n"
    "   \033[90mreport list\033[0m                                                                   list saved reports\n"
    "   \033[90mreport save <name>\033[0m                                                            save latest report under a name\n"
    "   \033[90mreport show <name>\033[0m                                                            print a saved reports\n"
    "   \033[90mreport delete <name>\033[0m                                                          delete a saved report\n";

}

void print_help() { std::cout << help_content; }

void print_lines(const std::vector<std::string>& lines) {
    for (const std::string& line : lines) {
        std::cout << line << '\n';
    }
}
