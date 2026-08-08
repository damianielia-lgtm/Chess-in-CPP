#include "executor.h"

#include <iostream>
#include <variant>

#include "../diagnostics/perft.h"
#include "../diagnostics/debugger.h"
#include "commands.h"
#include "session.h"
#include "display.h"

void execute_impl(const PositionShowCommand&, Session& session) { print_pos_info(session.current_position()); }
void execute_impl(const PositionStartposCommand&, Session& session) { session.reset_pos(); }
void execute_impl(const PositionFenCommand& cmd, Session& session) { session.set_pos(cmd.fen); }

void execute_impl(const MoveCommand& cmd, Session& session) { session.apply_uci_move(cmd.uci); }

void execute_impl(const PerftPresetCommand& cmd, Session& session) { std::cout << run_perft(cmd.preset, cmd.mode); }
void execute_impl(const DebugCommand& cmd, Session& session) { std::cout << debug_pos(session.fen(), cmd.depth); }

void execute(const Command& command, Session& session) {
    std::visit(
        [&session](const auto& concrete_command) {
            execute_impl(concrete_command, session);
        },
        command
    );
}
