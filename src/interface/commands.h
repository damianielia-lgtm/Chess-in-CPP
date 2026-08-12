#pragma once

#include <variant>
#include <string>
#include <optional>

#include "../diagnostics/presets.h"
#include "../application/game.h"

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
    DebugCommand
>;
