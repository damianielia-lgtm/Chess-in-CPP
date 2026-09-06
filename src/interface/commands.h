#pragma once

#include <variant>
#include <string>
#include <optional>
#include <filesystem>
#include <cstdint>

#include "../storage/presets.h"
#include "../game/game.h"
#include "../config.h"

struct HelpCommand {};

struct PlayLocalCommand { std::optional<TimeControl> time; };
struct PlayEngineCommand { Color player_color; };
struct ReplayCommand { std::string name; };
struct AnalyzeCommand {};

struct PositionShowCommand {};
struct PositionStartposCommand {};
struct PositionFenCommand { std::string fen; };
struct PositionSavedFenCommand { std::string name; };
struct MoveCommand { std::string move_string; };

struct PerftPresetCommand { Preset preset; };
struct BenchmarkPerftPresetCommand { Preset preset; };
struct BenchmarkEnginePresetCommand { Preset preset; };
struct PerftCommand { int depth; };
struct BenchmarkPerftCommand { int depth; };
struct BenchmarkEngineCommand { int depth; };
struct DebugCommand { int depth; };

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
struct ConfigSetEngineDepthCommand { int depth; };

struct EngineStaticCommand {};
struct EngineDynamicCommand {};
struct EngineBestmoveCommand {};
struct EngineRankMovesCommand {};

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
    
    PerftPresetCommand,
    BenchmarkPerftPresetCommand,
    BenchmarkEnginePresetCommand,
    PerftCommand,
    BenchmarkPerftCommand,
    BenchmarkEngineCommand,
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
    ConfigSetEngineDepthCommand,

    EngineStaticCommand,
    EngineDynamicCommand,
    EngineBestmoveCommand,
    EngineRankMovesCommand
>;
