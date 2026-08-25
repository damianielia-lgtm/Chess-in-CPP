#include "executor.h"

#include <variant>
#include <optional>
#include <string_view>

#include "../core/position.h"
#include "../diagnostics/perft.h"
#include "../diagnostics/debugger.h"
#include "../game/game_loop.h"
#include "../game/game.h"
#include "../storage/file_manager.h"
#include "../notation/pgn.h"
#include "commands.h"
#include "display.h"
#include "session.h"

namespace {

void execute_impl(const HelpCommand&, Session&) { print_help(); }

void execute_impl(const PlayCommand& cmd, Session& session) {
    std::optional<GameState> game = play_local(cmd.time);
    if (game.has_value()) { session.store_last_game(std::move(game.value())); }
}

void execute_impl(const PositionShowCommand&, Session& session) { print_pos_info(session.current_position()); }
void execute_impl(const PositionStartposCommand&, Session& session) { session.reset_pos(); }
void execute_impl(const PositionFenCommand& cmd, Session& session) { session.set_pos(cmd.fen); }

void execute_impl(const MoveCommand& cmd, Session& session) { session.apply_uci_move(cmd.uci); }

void execute_impl(const PerftPresetCommand& cmd, Session&) { print_lines(run_test_preset(cmd.preset)); }
void execute_impl(const BenchmarkPerftPresetCommand& cmd, Session&) { print_lines(run_benchmark_preset(cmd.preset)); }
void execute_impl(const PerftCommand& cmd, Session& session) {
    Position position = session.current_position();
    print_lines(run_test(position, cmd.depth));
}
void execute_impl(const BenchmarkPerftCommand& cmd, Session& session) {
    Position position = session.current_position();
    print_lines(run_benchmark(position, cmd.depth));
}
void execute_impl(const DebugCommand& cmd, Session& session) {
    print_lines(debug_pos(session.current_position().to_fen(), cmd.depth));
}

void execute_impl(const PgnDeleteCommand& cmd, Session&) {
    std::filesystem::path dir = make_pgn_path(cmd.name);
    delete_file(dir);
}
void execute_impl(const PgnSaveCommand& cmd, Session& session) {
    std::filesystem::path dir = make_pgn_path(cmd.name);
    std::vector<std::string> pgn_lines = construct_pgn_lines(session.last_game());
    write_file(dir, pgn_lines);
}
void execute_impl(const PgnShowCommand& cmd, Session& session) {
    std::filesystem::path dir = make_pgn_path(cmd.name);
    print_lines(read_file(dir));
}
void execute_impl(const PgnListCommand&, Session&) {
    std::vector<std::string> list = pgn_list();
    if (list.empty()) {
        print_lines({"No saved PGN's yet."});
    } else {
        print_lines(list);
    }
}

}

void execute(const Command& command, Session& session) {
    std::visit(
        [&session](const auto& concrete_command) {
            execute_impl(concrete_command, session);
        },
        command
    );
}
