#include "executor.h"

#include <variant>
#include <optional>
#include <string_view>
#include <filesystem>

#include "../core/position.h"
#include "../diagnostics/perft.h"
#include "../diagnostics/debugger.h"
#include "../game/game_loop.h"
#include "../game/game.h"
#include "../storage/file_manager.h"
#include "../notation/pgn.h"
#include "../notation/uci.h"
#include "../notation/san.h"
#include "../config.h"
#include "../errors.h"
#include "commands.h"
#include "display.h"
#include "session.h"

namespace fs = std::filesystem;

namespace {

void execute_impl(const HelpCommand&, Session&, ConfigData&) { print_help(); }

void execute_impl(const PlayCommand& cmd, Session& session, ConfigData& config) {
    std::optional<Game> game = play_local(cmd.time, config);
    if (game.has_value()) { session.store_last_game(std::move(*game)); }
}
void execute_impl(const ReplayCommand& cmd, Session&, ConfigData& config) {
    fs::path dir = make_pgn_path(cmd.name);
    std::vector<std::string> pgn_lines = read_file(dir);
    ParsedPGN parsed = parse_pgn_document(pgn_lines);
    Game game = reconstruct_game(parsed);
    replay(game, config);
}
void execute_impl(const AnalyzeCommand&, Session& session, ConfigData& config) {
    std::optional<Game> game = analyze(session.current_position(), config, false);
    if (game.has_value()) { session.store_last_game(std::move(*game)); }
}

void execute_impl(const PositionShowCommand&, Session& session, ConfigData& config) {
    print_lines(construct_board_lines(session.current_position(), config.board_orientation));
    print_lines({"Fen: \"" + session.current_position().to_fen() + '\"'});
}
void execute_impl(const PositionStartposCommand&, Session& session, ConfigData&) { session.reset_pos(); }
void execute_impl(const PositionFenCommand& cmd, Session& session, ConfigData&) { session.set_pos(cmd.fen); }
void execute_impl(const PositionSavedFenCommand& cmd, Session& session, ConfigData&) {
    fs::path dir = make_fen_path(cmd.name);
    std::vector<std::string> lines = read_file(dir);
    if (lines.size() != 1) { throw FenError("Saved fen must be one line."); }
    session.set_pos(lines[0]);
}
void execute_impl(const MoveCommand& cmd, Session& session, ConfigData& config) {
    session.apply_move(
        config.move_input == MoveInput::Uci
            ? resolve_uci(session.current_position(), cmd.move_string)
            : resolve_san(session.current_position(), cmd.move_string)
    );
}

void execute_impl(const PerftPresetCommand& cmd, Session& session, ConfigData&) {
    std::vector<std::string> lines = run_test_preset(cmd.preset);
    print_lines(lines);
    session.store_last_report(lines);
}
void execute_impl(const BenchmarkPerftPresetCommand& cmd, Session& session, ConfigData&) {
    std::vector<std::string> lines = run_benchmark_preset(cmd.preset);
    print_lines(lines);
    session.store_last_report(lines);
}
void execute_impl(const PerftCommand& cmd, Session& session, ConfigData&) {
    Position position = session.current_position();
    print_lines(run_test(position, cmd.depth));
}
void execute_impl(const BenchmarkPerftCommand& cmd, Session& session, ConfigData&) {
    Position position = session.current_position();
    print_lines(run_benchmark(position, cmd.depth));
}
void execute_impl(const DebugCommand& cmd, Session& session, ConfigData&) {
    print_lines(debug_pos(session.current_position().to_fen(), cmd.depth));
}

void execute_impl(const PgnDeleteCommand& cmd, Session&, ConfigData&) {
    fs::path dir = make_pgn_path(cmd.name);
    delete_file(dir);
}
void execute_impl(const PgnSaveCommand& cmd, Session& session, ConfigData& config) {
    fs::path dir = make_pgn_path(cmd.name);
    std::vector<std::string> pgn_lines = construct_pgn_lines(session.last_game(), config.pgn_save_clock);
    write_file(dir, pgn_lines);
}
void execute_impl(const PgnShowCommand& cmd, Session& session, ConfigData&) {
    fs::path dir = make_pgn_path(cmd.name);
    print_lines(read_file(dir));
}
void execute_impl(const PgnListCommand&, Session&, ConfigData&) {
    std::vector<std::string> files_list = pgn_list();
    print_lines(files_list.empty() ? std::vector<std::string>{"No saved PGN's yet."} : files_list);
}
void execute_impl(const PgnImportCommand& cmd, Session&, ConfigData&) {    
    std::vector<std::string> external_lines = read_file(cmd.path);
    reconstruct_game(parse_pgn_document(external_lines));

    fs::path local_dir = make_pgn_path(cmd.path.stem().string());
    write_file(local_dir, external_lines);
}
void execute_impl(const PgnExportCommand& cmd, Session&, ConfigData&) {    
    fs::path local_path = make_pgn_path(cmd.name);
    std::vector<std::string> lines = read_file(local_path);
    fs::path external_path = cmd.directory / local_path.filename();
    write_file(external_path, lines);
}

void execute_impl(const FenDeleteCommand& cmd, Session&, ConfigData&) {
    fs::path dir = make_fen_path(cmd.name);
    delete_file(dir);
}
void execute_impl(const FenSaveCommand& cmd, Session& session, ConfigData&) {
    fs::path dir = make_fen_path(cmd.name);
    std::string fen_line = session.current_position().to_fen();
    write_file(dir, {fen_line});
}
void execute_impl(const FenShowCommand& cmd, Session& session, ConfigData&) {
    fs::path dir = make_fen_path(cmd.name);
    print_lines(read_file(dir));
}
void execute_impl(const FenListCommand&, Session&, ConfigData&) {
    std::vector<std::string> files_list = fen_list();
    print_lines(files_list.empty() ? std::vector<std::string>{"No saved FEN's yet."} : files_list);
}
void execute_impl(const FenImportCommand& cmd, Session&, ConfigData&) {
    std::vector<std::string> external_lines = read_file(cmd.path);
    if (external_lines.size() != 1) { throw FenError("Saved fen must be one line."); }
    Position(external_lines[0]);

    fs::path local_dir = make_fen_path(cmd.path.stem().string());
    write_file(local_dir, external_lines);
}
void execute_impl(const FenExportCommand& cmd, Session&, ConfigData&) {
    fs::path local_path = make_fen_path(cmd.name);
    std::vector<std::string> lines = read_file(local_path);
    fs::path external_path = cmd.directory / local_path.filename();
    write_file(external_path, lines);
}

void execute_impl(const ReportDeleteCommand& cmd, Session&, ConfigData&) {
    fs::path dir = make_report_path(cmd.name);
    delete_file(dir);
}
void execute_impl(const ReportSaveCommand& cmd, Session& session, ConfigData&) {
    fs::path dir = make_report_path(cmd.name);
    std::vector<std::string> report_lines = session.last_report();
    write_file(dir, report_lines);
}
void execute_impl(const ReportShowCommand& cmd, Session& session, ConfigData&) {
    fs::path dir = make_report_path(cmd.name);
    print_lines(read_file(dir));
}
void execute_impl(const ReportListCommand&, Session&, ConfigData&) {
    std::vector<std::string> files_list = report_list();
    print_lines(files_list.empty() ? std::vector<std::string>{"No saved reports yet."} : files_list);
}

void execute_impl(const ConfigShowCommand&, Session&, ConfigData& config) {
    std::vector<std::string> config_lines = construct_config_show_lines(config);
    print_lines(config_lines);
}
void execute_impl(const ConfigSetPlayer1Command& cmd, Session&, ConfigData& config) { config.white_name = cmd.name; }
void execute_impl(const ConfigSetPlayer2Command& cmd, Session&, ConfigData& config) { config.black_name = cmd.name; }
void execute_impl(const ConfigSetEventCommand& cmd, Session&, ConfigData& config) { config.event = cmd.event; }
void execute_impl(const ConfigSetSiteCommand cmd, Session&, ConfigData& config) { config.site = cmd.site; }
void execute_impl(const ConfigSetExportClocksCommand& cmd, Session&, ConfigData& config) { config.pgn_save_clock = cmd.export_cloks; }
void execute_impl(const ConfigSetMoveInputCommand& cmd, Session&, ConfigData& config) { config.move_input = cmd.input; }
void execute_impl(const ConfigSetBoardOrientationCommand& cmd, Session&, ConfigData& config) { config.board_orientation = cmd.orientation; }

}

void execute(const Command& command, Session& session, ConfigData& config) {
    std::visit(
        [&session, &config](const auto& concrete_command) {
            execute_impl(concrete_command, session, config);
        },
        command
    );
}
