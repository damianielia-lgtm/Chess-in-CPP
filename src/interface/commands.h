#pragma once

#include <variant>
#include <string>

struct PositionShowCommand {};

struct PositionStartposCommand {};

struct PositionFenCommand {
    std::string fen;
};


struct MoveCommand {
    std::string uci;
};


enum class Preset {
    Instant,
    Fast,
    Moderate,
    Extended
};

enum class PerftMode {
    Test,
    Benchmark
};

struct PerftPresetCommand {
    Preset preset;
    PerftMode mode;
};

struct DebugCommand {
    int depth;
};


using Command = std::variant<
    PositionShowCommand,
    PositionStartposCommand,
    PositionFenCommand,
    MoveCommand,
    PerftPresetCommand,
    DebugCommand
>;
