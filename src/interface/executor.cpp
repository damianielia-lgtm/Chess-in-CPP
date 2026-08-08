#include "executor.h"

#include <iostream>
#include <variant>
#include <string_view>

#include "../diagnostics/perft.h"
#include "../diagnostics/debugger.h"
#include "commands.h"
#include "session.h"
#include "display.h"

constexpr std::string_view help_content = 
    "\033[1mCore\033[0m\n"
    "   \033[90mposition show\033[0m                                                                 print current position\n"
    "   \033[90mposition --startpos\033[0m                                                           set starting position\n"
    "   \033[90mposition --fen \"<fen string>\"\033[0m                                                   load in specific fen\n"
    "   \033[90mmove <uci move>\033[0m                                                               apply uci move to current position\n\n"

    "\033[1mGame features\033[0m\n"
    "   \033[90mplay local [--time-control <initial+increment>] [--save <pgn name>]\033[0m           start a local game, both sides on this machine\n"
    "   \033[90mplay online\033[0m                                                                   start an online game\n"
    "   \033[90mplay engine --color {white|black} --depth <n> [--save <pgn name>]\033[0m             play against the engine\n"
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
    "   \033[90mfen list\033[0m list saved FENs\n"
    "   \033[90mfen save <name> \"<fen>\"\033[0m                                                       save a FEN string under a name\n"
    "   \033[90mfen show <name>\033[0m                                                               print a saved FEN\n"
    "   \033[90mfen delete <name>\033[0m                                                             delete a saved FEN\n"
    "   \033[90mfen import <file> --name <name>\033[0m                                               load a fen to game directory\n"
    "   \033[90mfen export <name> --output <file>\033[0m                                             export a saved fen from game directory\n\n"

    "\033[1mPerft testing\033[0m\n"
    "   \033[90mperft --preset <instant|fast|moderate|extended> [--output <file>]\033[0m             test engine corectness through the database\n"
    "   \033[90mperft --depth <n> [--output <file>]\033[0m                                           test corectness on a current position.\n"
    "   \033[90mdebug --depth <n> [--output <file>]\033[0m                                           recusively go through a position and compare with stockfish.\n\n"

    "\033[1mBenchmarking\033[0m\n"
    "   \033[90mbenchmark perft --preset <instant|fast|moderate|extended> [--output <file>]\033[0m   test movegen speed through the database\n"
    "   \033[90mbenchmark perft --depth <n> [--output <file>]\033[0m                                 test movegen speed on current position\n"
    "   \033[90mbenchmark engine --preset <instant|fast|moderate|extended> [--output <file>]\033[0m  test engine speed through the database\n"
    "   \033[90mbenchmark engine --depth <n> [--output <file>]\033[0m                                test engine speed on current position\n\n"

    "\033[1mReport management\033[0m\n"
    "   \033[90mreport list\033[0m                                                                   list saved reports\n"
    "   \033[90mreport show <name>\033[0m                                                            print a saved reports\n"
    "   \033[90mreport delete <name>\033[0m                                                          delete a saved report\n";

void execute_impl(const HelpCommand&, Session& session) { std::cout << help_content; }

void execute_impl(const PositionShowCommand&, Session& session) { print_pos_info(session.current_position()); }
void execute_impl(const PositionStartposCommand&, Session& session) { session.reset_pos(); }
void execute_impl(const PositionFenCommand& cmd, Session& session) { session.set_pos(cmd.fen); }

void execute_impl(const MoveCommand& cmd, Session& session) { session.apply_uci_move(cmd.uci); }

void execute_impl(const PerftPresetCommand& cmd, Session& session) { std::cout << run_test_preset(cmd.preset); }
void execute_impl(const BenchmarkPerftPresetCommand& cmd, Session& session) { std::cout << run_benchmark_preset(cmd.preset); }
void execute_impl(const PerftCommand& cmd, Session& session) { std::cout << run_test(session.current_position(), cmd.depth); }
void execute_impl(const BenchmarkPerftCommand& cmd, Session& session) { std::cout << run_benchmark(session.current_position(), cmd.depth); }
void execute_impl(const DebugCommand& cmd, Session& session) { std::cout << debug_pos(session.current_position().to_fen(), cmd.depth); }

void execute(const Command& command, Session& session) {
    std::visit(
        [&session](const auto& concrete_command) {
            execute_impl(concrete_command, session);
        },
        command
    );
}
