#include "debugger.h"

#include <vector>
#include <string>
#include <iostream>
#include <sstream>
#include <map>

#include "stockfish_bridge.h"
#include "perft.h"
#include "../core/position.h"
#include "../core/move.h"
#include "../movegen/legal_moves.h"

std::map<std::string, uint64_t> stockfish_results(StockfishProcess& sf, std::string fen, int depth) {
    sendCommand(sf, "position fen " + fen);
    sendCommand(sf, "go perft " + std::to_string(depth));
    std::string unparsed_sf_output = readUntil(sf, "Nodes searched:");

    std::stringstream ss(unparsed_sf_output);
    std::string token;
    std::map<std::string, uint64_t> sf_output;
    
    while (std::getline(ss, token, '\n')) {
        const std::size_t delimiter_pos = token.rfind(": ");

        if (delimiter_pos == std::string::npos) { continue; }

        const std::string move_text = token.substr(0, delimiter_pos);
        const std::string count_text = token.substr(delimiter_pos + 2);

        if (move_text.size() != 4 && move_text.size() != 5) { continue; }

        sf_output[move_text] = std::stoull(count_text);
    }

    return sf_output;
}

std::map<std::string, uint64_t> debug_perft(std::string fen_string, int depth) {
    Position position(fen_string);
    Position original_pos = position;
    std::map<std::string, uint64_t> divide;

    for (const Move move : all_moves(position, false)) {
        UndoState move_state = position.apply_move(move);
        divide[move.to_uci()] = perft(position, depth - 1);
        position.revert_move(move, move_state);

        if (position != original_pos) {
            throw std::runtime_error(
                "undo move bug: applying and reverting move '" + move.to_uci()
                + "' from fen '" + fen_string + "', got '" + position.to_fen() + "'.\n");
        }
    }

    return divide;
}

std::string stockfish_apply_move(StockfishProcess& sf, std::string fen, std::string move) {
    sendCommand(sf, "position fen " + fen + " moves " + move);
    sendCommand(sf, "d");
    std::string unparsed_sf_output = readUntil(sf, "Checkers:");

    std::stringstream ss(unparsed_sf_output);
    std::string token;
    while (std::getline(ss, token, '\n')) {
        if (token.starts_with("Fen: ")) {
            return token.substr(5);
        }
    }

    throw std::runtime_error("Unexpected Stockfish behavior.");
}

std::string debugger(StockfishProcess& sf, std::string fen, int depth) {
    std::map<std::string, uint64_t> my_engine;
    try {
        my_engine = debug_perft(fen, depth);
    } catch (const std::runtime_error& e) {
        return e.what();
    }
    std::map<std::string, uint64_t> stockfish = stockfish_results(sf, fen, depth);

    for (const auto& [move, nodes] : my_engine) {
        if (!stockfish.contains(move)) {
            return "excess node: " + move + " (final fen: '" + fen + "')\n";
        }
    }
    for (const auto& [move, nodes] : stockfish) {
        if (!my_engine.contains(move)) {
            return "missing node: " + move + " (final fen: '" + fen + "')\n";
        }
    }

    if (depth == 1) {
        for (const auto& [move, nodes] : my_engine) {
            if (nodes != 1) {
                std::string bug_message;
                bug_message += "depth 1 mismatch: move '" + move + "' gives " + std::to_string(nodes);
                bug_message += " instead of 1 (final fen: '" + fen + "')\n";
                return bug_message;
            }
        }
        return "No bugs found on this position!\n";
    }

    for (const auto& [uci_move, nodes] : my_engine) {
        if (stockfish[uci_move] != nodes) {
            Position pos(fen);
            Position before_simulation = pos;
            Move move = pos.resolve_uci(uci_move);

            UndoState move_state = pos.apply_move(move);
            std::string child_fen = pos.to_fen();
            std::string stockfish_fen = stockfish_apply_move(sf, fen, uci_move);

            if (child_fen != stockfish_fen) {
                std::string bug_message;
                bug_message += "do move bug: applying move '" + uci_move + "' to fen '" + fen;
                bug_message += "' (expected '" + stockfish_fen + "', got '" + child_fen + "')\n";
                return bug_message;
            }

            pos.revert_move(move, move_state);
            if (pos != before_simulation) {
                std::string bug_message;
                bug_message += "undo move bug: reverting move '" + uci_move + "' from fen '" + child_fen;
                bug_message += "' (expected '" + fen + "', got '" + pos.to_fen() + "')\n";
                return bug_message;
            }

            return uci_move + " " + debugger(sf, child_fen, depth - 1);
        }
    }

    return "No bugs found on this position!\n";
}

std::string debug_pos(std::string fen, int depth) {
    StockfishProcess sf = startStockfish(
        L"C:\\Users\\User\\Downloads\\stockfish-windows-x86-64-avx2\\stockfish\\stockfish-windows-x86-64-avx2.exe"
    );

    sendCommand(sf, "uci");
    readUntil(sf, "uciok");
    sendCommand(sf, "isready");
    readUntil(sf, "readyok");

    if (fen == "startpos") { fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"; }
    std::string debug_lines = debugger(sf, fen, depth);

    sf.requestQuit();

    return debug_lines;
}
