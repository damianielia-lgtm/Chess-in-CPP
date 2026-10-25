#include "cli.h"

#include <variant>
#include <optional>
#include <string_view>
#include <filesystem>
#include <utility>

#include "../core/position.h"
#include "../diagnostics/diagnostics.h"
#include "../game/game_loop.h"
#include "../game/game.h"
#include "../storage/file_manager.h"
#include "../notation/pgn.h"
#include "../notation/move_notation.h"
#include "../engine/evaluation.h"
#include "../engine/search.h"
#include "../config.h"
#include "../errors.h"
#include "display.h"
#include "session.h"

using namespace cli;
namespace fs = std::filesystem;

namespace {

void execute_impl(const Help&, CliSession&, ConfigData&) { print_help(); }


void execute_impl(const PlayLocal& cmd, CliSession& session, ConfigData& config) {
    std::optional<Game> game = play_local(cmd.time, config);
    if (game.has_value()) { session.store_last_game(std::move(*game)); }
}

void execute_impl(const PlayEngine& cmd, CliSession& session, ConfigData& config) {
    std::optional<Game> game = play_engine(cmd.time, cmd.player_color, config);
    if (game.has_value()) { session.store_last_game(std::move(*game)); }
}

void execute_impl(const Replay& cmd, CliSession&, ConfigData& config) {
    fs::path dir = make_pgn_path(cmd.name);
    std::vector<std::string> pgn_lines = read_file(dir);
    ParsedPGN parsed = parse_pgn_document(pgn_lines);
    Game game = reconstruct_game(parsed);
    replay(game, config);
}

void execute_impl(const Analyze&, CliSession& session, ConfigData& config) {
    std::optional<Game> game = analyze(session.current_position(), config, false);
    if (game.has_value()) { session.store_last_game(std::move(*game)); }
}


void execute_impl(const PositionShow&, CliSession& session, ConfigData& config) {
    print_board(session.current_position(), config.board_orientation);
    print_lines({"Fen: \"" + session.current_position().to_fen() + '\"'});
}

void execute_impl(const PositionStartpos&, CliSession& session, ConfigData&) {
    session.reset_pos();
}

void execute_impl(const PositionFen& cmd, CliSession& session, ConfigData&) {
    session.set_pos(cmd.fen);
}

void execute_impl(const PositionSavedFen& cmd, CliSession& session, ConfigData&) {
    std::vector<std::string> lines = read_file(make_fen_path(cmd.name));
    if (lines.size() != 1) { throw FenError("Saved fen must be one line."); }
    session.set_pos(lines[0]);
}

void execute_impl(const ApplyMove& cmd, CliSession& session, ConfigData& config) {
    session.apply_move(resolve_move(cmd.move_string, session.current_position(), config.move_notation));
}


void print_and_save(ReportContents report, CliSession& session) {
    print_lines(report.text_lines);
    session.store_last_report(std::move(report));
}


void execute_impl(const PerftTest& cmd, CliSession& session, ConfigData& config) {
    Position position = session.current_position();
    print_and_save(perft_test(position, cmd.depth, config.move_notation), session);
}

void execute_impl(const PerftTestPreset& cmd, CliSession& session, ConfigData&) {
    print_and_save(perft_test_preset(cmd.preset), session);
}

void execute_impl(const PerftBenchmark& cmd, CliSession& session, ConfigData&) {
    Position position = session.current_position();
    print_and_save(perft_benchmark(position, cmd.depth), session);
}

void execute_impl(const PerftBenchmarkPreset& cmd, CliSession& session, ConfigData&) {
    print_and_save(perft_benchmark_preset(cmd.preset), session);
}

void execute_impl(const EngineBenchmark& cmd, CliSession& session, ConfigData& config) {
    Position position = session.current_position();
    print_and_save(benchmark_engine(position, cmd.depth, config), session);
}

void execute_impl(const EngineBenchmarkPreset& cmd, CliSession& session, ConfigData& config) {
    print_and_save(benchmark_engine_preset(cmd.preset, config), session);
}

void execute_impl(const EngineProfilePruning& cmd, CliSession& session, ConfigData&) {
    Position position = session.current_position();
    print_and_save(profile_pruning(position, cmd.depth), session);
}

void execute_impl(const EngineProfilePruningPreset& cmd, CliSession& session, ConfigData&) {
    print_and_save(profile_pruning_preset(cmd.preset), session);
}

void execute_impl(const Debug& cmd, CliSession& session, ConfigData&) {
    print_and_save(debug_pos(session.current_position().to_fen(), cmd.depth), session);
}


void execute_impl(const PgnDelete& cmd, CliSession&, ConfigData&) {
    fs::path dir = make_pgn_path(cmd.name);
    delete_file(dir);
}

void execute_impl(const PgnSave& cmd, CliSession& session, ConfigData& config) {
    fs::path dir = make_pgn_path(cmd.name);
    std::vector<std::string> pgn_lines = construct_pgn_lines(session.last_game(), config.pgn_save_clock);
    write_file(dir, pgn_lines);
}

void execute_impl(const PgnShow& cmd, CliSession& session, ConfigData&) {
    fs::path dir = make_pgn_path(cmd.name);
    print_lines(read_file(dir));
}

void execute_impl(const PgnList&, CliSession&, ConfigData&) {
    std::vector<std::string> files_list = pgn_list();
    print_lines(files_list.empty() ? std::vector<std::string>{"No saved PGN's yet."} : files_list);
}

void execute_impl(const PgnImport& cmd, CliSession&, ConfigData&) {    
    std::vector<std::string> external_lines = read_file(cmd.path);
    reconstruct_game(parse_pgn_document(external_lines));

    fs::path local_dir = make_pgn_path(cmd.path.stem().string());
    write_file(local_dir, external_lines);
}

void execute_impl(const PgnExport& cmd, CliSession&, ConfigData&) {    
    fs::path local_path = make_pgn_path(cmd.name);
    std::vector<std::string> lines = read_file(local_path);
    fs::path external_path = cmd.directory / local_path.filename();
    write_file(external_path, lines);
}


void execute_impl(const FenDelete& cmd, CliSession&, ConfigData&) {
    fs::path dir = make_fen_path(cmd.name);
    delete_file(dir);
}

void execute_impl(const FenSave& cmd, CliSession& session, ConfigData&) {
    fs::path dir = make_fen_path(cmd.name);
    std::string fen_line = session.current_position().to_fen();
    write_file(dir, {fen_line});
}

void execute_impl(const FenShow& cmd, CliSession& session, ConfigData&) {
    fs::path dir = make_fen_path(cmd.name);
    print_lines(read_file(dir));
}

void execute_impl(const FenList&, CliSession&, ConfigData&) {
    std::vector<std::string> files_list = fen_list();
    print_lines(files_list.empty() ? std::vector<std::string>{"No saved FEN's yet."} : files_list);
}

void execute_impl(const FenImport& cmd, CliSession&, ConfigData&) {
    std::vector<std::string> external_lines = read_file(cmd.path);
    if (external_lines.size() != 1) { throw FenError("Saved fen must be one line."); }
    Position{external_lines[0]};

    fs::path local_dir = make_fen_path(cmd.path.stem().string());
    write_file(local_dir, external_lines);
}

void execute_impl(const FenExport& cmd, CliSession&, ConfigData&) {
    fs::path local_path = make_fen_path(cmd.name);
    std::vector<std::string> lines = read_file(local_path);
    fs::path external_path = cmd.directory / local_path.filename();
    write_file(external_path, lines);
}


void execute_impl(const ReportDelete& cmd, CliSession&, ConfigData&) {
    delete_file(make_report_path(cmd.name));
    try {
        delete_file(make_csv_path(cmd.name));
    } catch (StorageError&) {}
}

void execute_impl(const ReportSave& cmd, CliSession& session, ConfigData&) {
    const ReportContents& report = session.last_report();
    const fs::path text_path = make_report_path(cmd.name);
    const fs::path csv_path = make_csv_path(cmd.name);

    ensure_path_available(text_path);
    if (report.csv_lines) { ensure_path_available(csv_path); }

    write_file(text_path, report.text_lines);
    if (report.csv_lines) { write_file(csv_path, *report.csv_lines); }
}

void execute_impl(const ReportShow& cmd, CliSession& session, ConfigData&) {
    fs::path dir = make_report_path(cmd.name);
    print_lines(read_file(dir));
}

void execute_impl(const ReportList&, CliSession&, ConfigData&) {
    std::vector<std::string> files_list = report_list();
    print_lines(files_list.empty() ? std::vector<std::string>{"No saved reports yet."} : files_list);
}


void execute_impl(const ConfigShow&, CliSession&, ConfigData& config) {
    print_config_show(config);
}

void execute_impl(const ConfigSetPlayer1& cmd, CliSession&, ConfigData& config) {
    config.player1_name = cmd.name;
}

void execute_impl(const ConfigSetPlayer2& cmd, CliSession&, ConfigData& config) {
    config.player2_name = cmd.name;
}

void execute_impl(const ConfigSetEvent& cmd, CliSession&, ConfigData& config) {
    config.event = cmd.event;
}

void execute_impl(const ConfigSetSite cmd, CliSession&, ConfigData& config) {
    config.site = cmd.site;
}

void execute_impl(const ConfigSetExportClocks& cmd, CliSession&, ConfigData& config) {
    config.pgn_save_clock = cmd.export_clocks;
}

void execute_impl(const ConfigSetMoveNotation& cmd, CliSession&, ConfigData& config) {
    config.move_notation = cmd.input;
}

void execute_impl(const ConfigSetBoardOrientation& cmd, CliSession&, ConfigData& config) {
    config.board_orientation = cmd.orientation;
}

void execute_impl(const ConfigSetBenchmarkStatsTracking& cmd, CliSession&, ConfigData& config) {
    config.track_stats = cmd.track_stats;
}


void execute_impl(const EngineStaticEval&, CliSession& session, ConfigData&) {
    std::int16_t eval = static_eval(session.current_position());
    print_lines({"Static eval : " + std::to_string(normalize_centipawn(eval))});
}

void execute_impl(const EngineDynamicEvalTime& cmd, CliSession& session, ConfigData&) {
    Position position = session.current_position();
    std::int16_t eval = pbm_fixed_time(position, cmd.thinking_time).eval;
    print_lines({"Dynamic eval : " + std::to_string(eval)});
}

void execute_impl(const EngineDynamicEvalDepth& cmd, CliSession& session, ConfigData&) {
    Position position = session.current_position();
    std::int16_t eval = pbm_fixed_depth(position, cmd.depth).eval;
    print_lines({"Dynamic eval : " + std::to_string(eval)});
}

void execute_impl(const EngineBestmoveTime& cmd, CliSession& session, ConfigData& config) {
    Position position = session.current_position();
    const std::optional<Move> bestmove = pbm_fixed_time(position, cmd.thinking_time).move;
    std::string line = bestmove
        ? "Best move : " + move_notation(*bestmove, position, config.move_notation)
        : "No moves available.";
    print_lines({line});
}

void execute_impl(const EngineBestmoveDepth& cmd, CliSession& session, ConfigData& config) {
    Position position = session.current_position();
    const std::optional<Move> bestmove = pbm_fixed_depth(position, cmd.depth).move;
    std::string line = bestmove
        ? "Best move : " + move_notation(*bestmove, position, config.move_notation)
        : "No moves available.";
    print_lines({line});
}

void execute_impl(const EngineRankMoves& cmd, CliSession& session, ConfigData& config) {
    Position position = session.current_position();
    std::vector<SearchedMove> moves = rank_moves(position, cmd.depth);

    std::vector<std::string> lines;
    int rank = 1;
    for (const SearchedMove& move : moves) {
        lines.push_back(
            std::to_string(rank) + ". " +
            move_notation(*move.move, position, config.move_notation) +
            ": " + std::to_string(move.eval)
        );

        rank++;
    }
    print_lines(lines);
}

}

void execute_cli(const Command& command, CliSession& session, ConfigData& config) {
    std::visit(
        [&session, &config](const auto& concrete_command) {
            execute_impl(concrete_command, session, config);
        },
        command
    );
}
