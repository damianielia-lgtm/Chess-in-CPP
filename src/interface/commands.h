#pragma once

#include <variant>
#include <string>
#include <optional>

#include "../storage/presets.h"
#include "../game/game.h"
#include "../config.h"

struct HelpCommand {};

struct PlayCommand { std::optional<TimeControl> time; };
struct ReplayCommand { std::string name; };
struct AnalyzeCommand {};

struct PositionShowCommand {};
struct PositionStartposCommand {};
struct PositionFenCommand { std::string fen; };
struct PositionSavedFenCommand { std::string name; };
struct MoveCommand { std::string move_string; };

struct PerftPresetCommand { Preset preset; };
struct BenchmarkPerftPresetCommand { Preset preset; };
struct PerftCommand { int depth; };
struct BenchmarkPerftCommand { int depth; };
struct DebugCommand { int depth; };

struct PgnDeleteCommand { std::string name; };
struct PgnSaveCommand { std::string name; };
struct PgnShowCommand { std::string name; };
struct PgnListCommand {};

struct FenDeleteCommand { std::string name; };
struct FenSaveCommand { std::string name; };
struct FenShowCommand { std::string name; };
struct FenListCommand {};

struct ReportDeleteCommand { std::string name; };
struct ReportSaveCommand { std::string name; };
struct ReportShowCommand { std::string name; };
struct ReportListCommand {};

struct ConfigShowCommand {};
struct ConfigSetPlayer1Command { std::string name; };
struct ConfigSetPlayer2Command { std::string name; };
struct ConfigSetEventCommand { std::string event; };
struct ConfigSetSiteCommand { std::string site; };
struct ConfigSetExportClocksCommand { bool export_cloks; };
struct ConfigSetMoveInputCommand { MoveInput input; };
struct ConfigSetBoardOrientationCommand { BoardOrientation orientation; };

using Command = std::variant<
    HelpCommand,

    PlayCommand,
    ReplayCommand,
    AnalyzeCommand,

    PositionShowCommand,
    PositionStartposCommand,
    PositionFenCommand,
    PositionSavedFenCommand,
    MoveCommand,
    
    PerftPresetCommand,
    BenchmarkPerftPresetCommand,
    PerftCommand,
    BenchmarkPerftCommand,
    DebugCommand,

    PgnDeleteCommand,
    PgnSaveCommand,
    PgnShowCommand,
    PgnListCommand,

    FenDeleteCommand,
    FenSaveCommand,
    FenShowCommand,
    FenListCommand,
    
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
    ConfigSetMoveInputCommand,
    ConfigSetBoardOrientationCommand
>;
