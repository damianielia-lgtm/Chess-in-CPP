#pragma once

#include <variant>
#include <string>
#include <optional>
#include <cstdint>
#include <chrono>
#include <vector>

#include "session.h"

namespace uci {

struct IsReady {};
struct NewGame {};
struct PositionFen {
    std::string fen;
    std::optional<std::vector<std::string>> moves;
};

struct GoDepth { std::uint8_t depth; };
struct GoMoveTime { std::chrono::milliseconds time; };
struct GoTimedGame {
    std::chrono::milliseconds wtime;
    std::chrono::milliseconds btime;
    std::chrono::milliseconds winc;
    std::chrono::milliseconds binc;
    std::optional<int> mtg;
};

using Command = std::variant<
    IsReady,
    NewGame,
    PositionFen,

    GoDepth,
    GoMoveTime,
    GoTimedGame
>;

};

uci::Command parse_uci(std::string line);
void execute_uci(const uci::Command& command, UciSession& session);

inline void uci_command(std::string line, UciSession& session) {
    execute_uci(parse_uci(line), session);
}
