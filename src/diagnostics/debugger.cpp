#include "diagnostics.h"

#include <vector>
#include <string>
#include <sstream>
#include <map>
#include <expected>
#include <cstddef>
#include <utility>
#include <cstdint>
#include <optional>

#include "../core/position.h"
#include "../core/move.h"
#include "../movegen/legal_moves.h"
#include "../notation/move_notation.h"
#include "../storage/process.h"
#include "../errors.h"

namespace {

std::map<std::string, uint64_t> stockfish_results(
    Process& sf,
    std::string fen,
    std::uint8_t depth
) {
    sf.send_line("position fen " + fen);
    sf.send_line("go perft " + std::to_string(depth));
    std::vector<std::string> unparsed_sf_output = sf.read_until("Nodes searched:");

    std::map<std::string, uint64_t> sf_output;
    
    for (const std::string& line : unparsed_sf_output) {
        std::size_t delimiter_pos = line.rfind(": ");

        if (delimiter_pos == std::string::npos) { continue; }

        const std::string move_text = line.substr(0, delimiter_pos);
        const std::string count_text = line.substr(delimiter_pos + 2);

        if (move_text.size() != 4 && move_text.size() != 5) { continue; }

        sf_output[move_text] = std::stoull(count_text);
    }

    return sf_output;
}

std::expected<std::map<std::string, uint64_t>, std::string> debug_perft(
    std::string fen_string,
    std::uint8_t depth
) {
    Position position(fen_string);
    Position original_pos = position;
    std::map<std::string, uint64_t> divide;

    for (const Move move : all_moves(position)) {
        UndoState move_state = position.apply_move(move);
        divide[move_notation(move, position, MoveNotation::Uci)] = perft(position, depth - 1);
        position.revert_move(move, move_state);

        if (position != original_pos) {
            return std::unexpected(
                "undo move bug: applying and reverting move '" +
                move_notation(move, position, MoveNotation::Uci) +
                "' from fen '" + fen_string +
                "', got '" + position.to_fen() + "'.\n"
            );
        }
    }

    return divide;
}

std::string stockfish_apply_move(
    Process& sf,
    std::string fen,
    std::string move
) {
    sf.send_line("position fen " + fen + " moves " + move);
    sf.send_line("d");
    std::vector<std::string> unparsed_sf_output = sf.read_until("Checkers:");

    for (const std::string& line : unparsed_sf_output) {
        if (line.starts_with("Fen: ")) {
            return line.substr(5);
        }
    }

    throw ProcessError("Unexpected Stockfish behavior.");
}

std::string debugger(
    Process& sf,
    std::string fen,
    std::uint8_t depth
) {
    auto perft_result = debug_perft(fen, depth);

    if (!perft_result) {
        return perft_result.error();
    }

    std::map<std::string, uint64_t> my_engine = std::move(*perft_result);
    std::map<std::string, uint64_t> stockfish = stockfish_results(sf, fen, depth);

    for (const auto& [move, nodes] : my_engine) {
        if (!stockfish.contains(move)) {
            return "excess node: " + move + " (final fen: '" + fen + "')";
        }
    }
    for (const auto& [move, nodes] : stockfish) {
        if (!my_engine.contains(move)) {
            return "missing node: " + move + " (final fen: '" + fen + "')";
        }
    }

    if (depth == 1) {
        for (const auto& [move, nodes] : my_engine) {
            if (nodes != 1) {
                return "depth 1 mismatch: move '" + move + "' gives "
                    + std::to_string(nodes) + " instead of 1 (final fen: '"
                    + fen + "')";
            }
        }
        return "No bugs found on this position!";
    }

    for (const auto& [uci_move, nodes] : my_engine) {
        if (stockfish[uci_move] != nodes) {
            Position pos(fen);
            Position before_simulation = pos;
            Move move = resolve_move(uci_move, pos, MoveNotation::Uci);

            UndoState move_state = pos.apply_move(move);
            std::string child_fen = pos.to_fen();
            std::string stockfish_fen = stockfish_apply_move(sf, fen, uci_move);

            if (child_fen != stockfish_fen) {
                return "do move bug: applying move '" + uci_move
                    + "' to fen '" + fen + "' (expected '" + stockfish_fen
                    + "', got '" + child_fen + "')";
            }

            pos.revert_move(move, move_state);
            if (pos != before_simulation) {
                return "undo move bug: reverting move '" + uci_move
                    + "' from fen '" + child_fen + "' (expected '" + fen
                    + "', got '" + pos.to_fen() + "')";
            }

            return uci_move + " " + debugger(sf, child_fen, depth - 1);
        }
    }

    return "No bugs found on this position!";
}

}

ReportContents debug_pos(std::string fen, std::uint8_t depth) {
    Process sf("resources/stockfish.exe");

    sf.send_line("uci");
    sf.read_until("uciok");
    sf.send_line("isready");
    sf.read_until("readyok");

    if (fen == "startpos") { fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"; }

    std::vector<std::string> lines;

    lines.push_back("----- Debugger -----");
    lines.push_back("");

    lines.push_back("Suite: \"" + fen + "\" at depth " + std::to_string(depth));
    lines.push_back(debugger(sf, fen, depth));

    return {lines, std::nullopt};
}
