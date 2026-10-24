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

struct HelpCommand {};

struct PlayLocalCommand { std::optional<TimeControl> time; };
struct PlayEngineCommand { std::optional<TimeControl> time; Color player_color; };
struct ReplayCommand { std::string name; };
struct AnalyzeCommand {};

struct PositionShowCommand {};
struct PositionStartposCommand {};
struct PositionFenCommand { std::string fen; };
struct PositionSavedFenCommand { std::string name; };
struct MoveCommand { std::string move_string; };

struct PerftTestCommand { std::uint8_t depth; };
struct PerftTestPresetCommand { Preset preset; };
struct PerftBenchmarkCommand { std::uint8_t depth; };
struct PerftBenchmarkPresetCommand { Preset preset; };
struct EngineBenchmarkCommand { std::uint8_t depth; };
struct EngineBenchmarkPresetCommand { Preset preset; };
struct EngineProfilePruningCommand { std::uint8_t depth; };
struct EngineProfilePruningPresetCommand { Preset preset; };
struct DebugCommand { std::uint8_t depth; };

struct PgnDeleteCommand { std::string name; };
struct PgnSaveCommand { std::string name; };
struct PgnShowCommand { std::string name; };
struct PgnListCommand {};
struct PgnImportCommand { std::filesystem::path path; };
struct PgnExportCommand { std::string name; std::filesystem::path directory; };

struct FenDeleteCommand { std::string name; };
struct FenSaveCommand { std::string name; };
struct FenShowCommand { std::string name; };
struct FenListCommand {};
struct FenImportCommand { std::filesystem::path path; };
struct FenExportCommand { std::string name; std::filesystem::path directory; };

struct ReportDeleteCommand { std::string name; };
struct ReportSaveCommand { std::string name; };
struct ReportShowCommand { std::string name; };
struct ReportListCommand {};

struct ConfigShowCommand {};
struct ConfigSetPlayer1Command { std::string name; };
struct ConfigSetPlayer2Command { std::string name; };
struct ConfigSetEventCommand { std::string event; };
struct ConfigSetSiteCommand { std::string site; };
struct ConfigSetExportClocksCommand { bool export_clocks; };
struct ConfigSetMoveNotationCommand { MoveNotation input; };
struct ConfigSetBoardOrientationCommand { BoardOrientation orientation; };
struct ConfigSetBenchmarkStatsTrackingCommand { bool track_stats; };

struct EngineStaticEvalCommand {};
struct EngineDynamicEvalTimeCommand { std::chrono::milliseconds thinking_time; };
struct EngineDynamicEvalDepthCommand { std::uint8_t depth; };
struct EngineBestmoveTimeCommand { std::chrono::milliseconds thinking_time; };
struct EngineBestmoveDepthCommand { std::uint8_t depth; };
struct EngineRankMovesCommand { std::uint8_t depth; };

using Command = std::variant<
    HelpCommand,

    PlayLocalCommand,
    PlayEngineCommand,
    ReplayCommand,
    AnalyzeCommand,

    PositionShowCommand,
    PositionStartposCommand,
    PositionFenCommand,
    PositionSavedFenCommand,
    MoveCommand,

    PerftTestCommand,
    PerftTestPresetCommand,
    PerftBenchmarkCommand,
    PerftBenchmarkPresetCommand,
    EngineBenchmarkCommand,
    EngineBenchmarkPresetCommand,
    EngineProfilePruningCommand,
    EngineProfilePruningPresetCommand,
    DebugCommand,

    PgnDeleteCommand,
    PgnSaveCommand,
    PgnShowCommand,
    PgnListCommand,
    PgnImportCommand,
    PgnExportCommand,

    FenDeleteCommand,
    FenSaveCommand,
    FenShowCommand,
    FenListCommand,
    FenImportCommand,
    FenExportCommand,
    
    ReportDeleteCommand,
    ReportSaveCommand,
    ReportShowCommand,
    ReportListCommand,

    ConfigShowCommand,
    ConfigSetPlayer1Command,
    ConfigSetPlayer2Command,
    ConfigSetEventCommand,
    ConfigSetSiteCommand,
    ConfigSetExportClocksCommand,
    ConfigSetMoveNotationCommand,
    ConfigSetBoardOrientationCommand,
    ConfigSetBenchmarkStatsTrackingCommand,

    EngineStaticEvalCommand,
    EngineDynamicEvalTimeCommand,
    EngineDynamicEvalDepthCommand,
    EngineBestmoveTimeCommand,
    EngineBestmoveDepthCommand,
    EngineRankMovesCommand
>;

Command parse(std::string line);
void execute(const Command& command, Session& session, ConfigData& config);
