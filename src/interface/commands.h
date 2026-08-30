#pragma once

#include <variant>
#include <string>
#include <optional>

#include "../storage/presets.h"
#include "../game/game.h"

struct HelpCommand {};

struct PlayCommand { std::optional<TimeControl> time; };

struct PositionShowCommand {};
struct PositionStartposCommand {};
struct PositionFenCommand { std::string fen; };

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

struct ReplayCommand { std::string name; };

using Command = std::variant<
    HelpCommand,

    PlayCommand,

    PositionShowCommand,
    PositionStartposCommand,
    PositionFenCommand,

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

    ReplayCommand
>;
