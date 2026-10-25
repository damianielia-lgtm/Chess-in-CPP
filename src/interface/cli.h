#pragma once

#include <variant>
#include <string>
#include <optional>
#include <filesystem>
#include <cstdint>
#include <chrono>

#include "../storage/presets.h"
#include "../game/game.h"
#include "../config.h"
#include "session.h"

namespace cli {

struct Help {};

struct PlayLocal { std::optional<TimeControl> time; };
struct PlayEngine { std::optional<TimeControl> time; Color player_color; };
struct Replay { std::string name; };
struct Analyze {};

struct PositionShow {};
struct PositionStartpos {};
struct PositionFen { std::string fen; };
struct PositionSavedFen { std::string name; };
struct ApplyMove { std::string move_string; };

struct PerftTest { std::uint8_t depth; };
struct PerftTestPreset { Preset preset; };
struct PerftBenchmark { std::uint8_t depth; };
struct PerftBenchmarkPreset { Preset preset; };
struct EngineBenchmark { std::uint8_t depth; };
struct EngineBenchmarkPreset { Preset preset; };
struct EngineProfilePruning { std::uint8_t depth; };
struct EngineProfilePruningPreset { Preset preset; };
struct Debug { std::uint8_t depth; };

struct PgnDelete { std::string name; };
struct PgnSave { std::string name; };
struct PgnShow { std::string name; };
struct PgnList {};
struct PgnImport { std::filesystem::path path; };
struct PgnExport { std::string name; std::filesystem::path directory; };

struct FenDelete { std::string name; };
struct FenSave { std::string name; };
struct FenShow { std::string name; };
struct FenList {};
struct FenImport { std::filesystem::path path; };
struct FenExport { std::string name; std::filesystem::path directory; };

struct ReportDelete { std::string name; };
struct ReportSave { std::string name; };
struct ReportShow { std::string name; };
struct ReportList {};

struct ConfigShow {};
struct ConfigSetPlayer1 { std::string name; };
struct ConfigSetPlayer2 { std::string name; };
struct ConfigSetEvent { std::string event; };
struct ConfigSetSite { std::string site; };
struct ConfigSetExportClocks { bool export_clocks; };
struct ConfigSetMoveNotation { MoveNotation input; };
struct ConfigSetBoardOrientation { BoardOrientation orientation; };
struct ConfigSetBenchmarkStatsTracking { bool track_stats; };

struct EngineStaticEval {};
struct EngineDynamicEvalTime { std::chrono::milliseconds thinking_time; };
struct EngineDynamicEvalDepth { std::uint8_t depth; };
struct EngineBestmoveTime { std::chrono::milliseconds thinking_time; };
struct EngineBestmoveDepth { std::uint8_t depth; };
struct EngineRankMoves { std::uint8_t depth; };

using Command = std::variant<
    Help,

    PlayLocal,
    PlayEngine,
    Replay,
    Analyze,

    PositionShow,
    PositionStartpos,
    PositionFen,
    PositionSavedFen,
    ApplyMove,

    PerftTest,
    PerftTestPreset,
    PerftBenchmark,
    PerftBenchmarkPreset,
    EngineBenchmark,
    EngineBenchmarkPreset,
    EngineProfilePruning,
    EngineProfilePruningPreset,
    Debug,

    PgnDelete,
    PgnSave,
    PgnShow,
    PgnList,
    PgnImport,
    PgnExport,

    FenDelete,
    FenSave,
    FenShow,
    FenList,
    FenImport,
    FenExport,
    
    ReportDelete,
    ReportSave,
    ReportShow,
    ReportList,

    ConfigShow,
    ConfigSetPlayer1,
    ConfigSetPlayer2,
    ConfigSetEvent,
    ConfigSetSite,
    ConfigSetExportClocks,
    ConfigSetMoveNotation,
    ConfigSetBoardOrientation,
    ConfigSetBenchmarkStatsTracking,

    EngineStaticEval,
    EngineDynamicEvalTime,
    EngineDynamicEvalDepth,
    EngineBestmoveTime,
    EngineBestmoveDepth,
    EngineRankMoves
>;

};

cli::Command parse_cli(std::string line);
void execute_cli(const cli::Command& command, CliSession& session, ConfigData& config);

inline void cli_command(std::string line, CliSession& session, ConfigData& config) {
    execute_cli(parse_cli(line), session, config);
}
