#pragma once

#include <variant>
#include <string>
#include <optional>

#include "../storage/presets.h"
#include "../game/game.h"

struct HelpCommand {};

struct PlayCommand { std::optional<TimeControl> time; };
struct ReplayCommand { std::string name; };

struct PositionShowCommand {};
struct PositionStartposCommand {};
struct PositionFenCommand { std::string fen; };
struct PositionSavedFenCommand { std::string name; };
struct MoveCommand { std::string uci; };

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

using Command = std::variant<
    HelpCommand,

    PlayCommand,
    ReplayCommand,

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
    FenListCommand
>;
