#pragma once

#include <variant>
#include <string>

#include "../diagnostics/presets.h"

struct HelpCommand {};

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
