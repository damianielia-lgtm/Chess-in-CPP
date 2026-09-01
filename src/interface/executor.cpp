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
#include "../errors.h"
#include "commands.h"
#include "display.h"
#include "session.h"

namespace {

void execute_impl(const HelpCommand&, Session&) { print_help(); }

void execute_impl(const PlayCommand& cmd, Session& session) {
    std::optional<Game> game = play_local(cmd.time);
    if (game.has_value()) { session.store_last_game(std::move(*game)); }
}
void execute_impl(const ReplayCommand& cmd, Session&) {
    std::filesystem::path dir = make_pgn_path(cmd.name);
    std::vector<std::string> pgn_lines = read_file(dir);
    ParsedPGN parsed = parse_pgn_document(pgn_lines);
    Game game = reconstruct_game(parsed);
    replay(game);
}
void execute_impl(const AnalyzeCommand&, Session& session) {
    std::optional<Game> game = analyze(session.current_position(), false);
    if (game.has_value()) { session.store_last_game(std::move(*game)); }
}

void execute_impl(const PositionShowCommand&, Session& session) {
    print_lines(construct_board_lines(session.current_position(), false));
    print_lines({"Fen: \"" + session.current_position().to_fen() + '\"'});
}
void execute_impl(const PositionStartposCommand&, Session& session) { session.reset_pos(); }
void execute_impl(const PositionFenCommand& cmd, Session& session) { session.set_pos(cmd.fen); }
void execute_impl(const PositionSavedFenCommand& cmd, Session& session) {
    std::filesystem::path dir = make_fen_path(cmd.name);
    std::vector<std::string> lines = read_file(dir);
    if (lines.size() != 1) { throw FenError("Saved fen must be one line."); }
    session.set_pos(lines[0]);
}
void execute_impl(const MoveCommand& cmd, Session& session) { session.apply_uci_move(cmd.uci); }

void execute_impl(const PerftPresetCommand& cmd, Session& session) {
    std::vector<std::string> lines = run_test_preset(cmd.preset);
    print_lines(lines);
    session.store_last_report(lines);
}
void execute_impl(const BenchmarkPerftPresetCommand& cmd, Session& session) {
    std::vector<std::string> lines = run_benchmark_preset(cmd.preset);
    print_lines(lines);
    session.store_last_report(lines);
}
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
    std::vector<std::string> files_list = pgn_list();
    print_lines(files_list.empty() ? std::vector<std::string>{"No saved PGN's yet."} : files_list);
}

void execute_impl(const FenDeleteCommand& cmd, Session&) {
    std::filesystem::path dir = make_fen_path(cmd.name);
    delete_file(dir);
}
void execute_impl(const FenSaveCommand& cmd, Session& session) {
    std::filesystem::path dir = make_fen_path(cmd.name);
    std::string fen_line = session.current_position().to_fen();
    write_file(dir, {fen_line});
}
void execute_impl(const FenShowCommand& cmd, Session& session) {
    std::filesystem::path dir = make_fen_path(cmd.name);
    print_lines(read_file(dir));
}
void execute_impl(const FenListCommand&, Session&) {
    std::vector<std::string> files_list = fen_list();
    print_lines(files_list.empty() ? std::vector<std::string>{"No saved FEN's yet."} : files_list);
}

void execute_impl(const ReportDeleteCommand& cmd, Session&) {
    std::filesystem::path dir = make_report_path(cmd.name);
    delete_file(dir);
}
void execute_impl(const ReportSaveCommand& cmd, Session& session) {
    std::filesystem::path dir = make_report_path(cmd.name);
    std::vector<std::string> report_lines = session.last_report();
    write_file(dir, report_lines);
}
void execute_impl(const ReportShowCommand& cmd, Session& session) {
    std::filesystem::path dir = make_report_path(cmd.name);
    print_lines(read_file(dir));
}
void execute_impl(const ReportListCommand&, Session&) {
    std::vector<std::string> files_list = report_list();
    print_lines(files_list.empty() ? std::vector<std::string>{"No saved reports yet."} : files_list);
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
