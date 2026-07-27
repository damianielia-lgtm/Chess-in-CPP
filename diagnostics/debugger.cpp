#include <vector>
#include <string>
#include <iostream>
#include <sstream>
#include <map>
#include "stockfish_bridge.h"
#include "perft.h"
#include "..\app\conversion\fen.h"
#include "..\app\core\move_simulation.h"
#include "..\app\game_logic\legal_moves.h"
#include "..\app\core\encoding.h"

std::map<std::string, uint64_t> stockfish_results(StockfishProcess& sf, std::string fen, int depth) {
     sendCommand(sf, "position fen " + fen);
     sendCommand(sf, "go perft " + std::to_string(depth));
     std::string unparsed_sf_output = readUntil(sf, "Nodes searched:");

     std::stringstream ss(unparsed_sf_output);
     std::string token;
     std::map<std::string, uint64_t> sf_output;
     while (std::getline(ss, token, '\n')) {
          if (token.empty() || !token.contains(": ") || token.starts_with("Nodes searched:")) {continue;}
          size_t delimiter_pos = token.find(':');
          std::string key = token.substr(0, delimiter_pos);
          std::string value = token.substr(delimiter_pos + 2);
          sf_output[key] = std::stoull(value);
     }

     return sf_output;
}

bool match_pos(const Position& pos1, const Position& pos2) {
     if (pos1.board != pos2.board) {return false;}
     if (pos1.turn != pos2.turn) {return false;}
     if (pos1.castling_rights != pos2.castling_rights) {return false;}
     if (pos1.en_passant_target != pos2.en_passant_target) {return false;}
     if (pos1.halfmove_clock != pos2.halfmove_clock) {return false;}
     if (pos1.move_clock != pos2.move_clock) {return false;}
     if (pos1.white_king != pos2.white_king) {return false;}
     if (pos1.black_king != pos2.black_king) {return false;}
     return true;
}

std::map<std::string, uint64_t> perft_div(std::string fen_string, int depth) {
     Position position = from_fen(fen_string);
     Position original_pos = position;
     std::map<std::string, uint64_t> divide;
     MovesList legal_moves = all_legal_moves(position, false);
     for (int index = 0; index < legal_moves.count; index++) {
          int move = legal_moves.moves[index];
          undo_storage move_state = apply_move(move, position);
          divide[to_uci(move)] = perft(position, depth - 1);
          take_back_move(move, position, move_state);
          if (!match_pos(position, original_pos)) {
               throw std::runtime_error("undo move bug: applying and reverting move '" + to_uci(move) + "' from fen '" + fen_string + "', got '" + to_fen(position) + "'.\n");
          }
     }
     return divide;
}

int from_uci_with_flags(Position& pos, const std::string& uci) {
     MovesList legal_moves = all_legal_moves(pos, false);
     for (int i = 0; i < legal_moves.count; i++) {
          if (to_uci(legal_moves.moves[i]) == uci) {
               return legal_moves.moves[i];
          }
     }
     throw std::runtime_error("move_from_uci: '" + uci + "' not found in your legal move list");
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
          my_engine = perft_div(fen, depth);
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
                    return "depth 1 mismatch: move '" + move + "' gives " + std::to_string(nodes) + " instead of 1 (final fen: '" + fen + "')\n"
               }
          }
          return "No bugs found on this position!\n";
     }

     for (const auto& [move, nodes] : my_engine) {
          if (stockfish[move] != nodes) {
               Position pos = from_fen(fen);
               Position before_simulation = pos;
               int encoded_move = from_uci_with_flags(pos, move);
               undo_storage move_state = apply_move(encoded_move, pos);
               std::string child_fen = to_fen(pos);
               std::string stockfish_fen = stockfish_apply_move(sf, fen, move);
               if (child_fen != stockfish_fen) {
                    return "do move bug: applying move '" + move + "' to fen '" + fen + "' (expected '" + stockfish_fen + "', got '" + child_fen + "')\n";
               }
               take_back_move(encoded_move, pos, move_state);
               if (pos != before_simulation) {
                    return "undo move bug: reverting move '" + move + "' from fen '" + child_fen + "' (expected '" + fen + "', got '" + to_fen(pos) + "')\n";
               }
               return move + " " + debugger(sf, child_fen, depth - 1);
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

     if (fen == "startpos") {fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";}
     std::string debug_lines = debugger(sf, fen, depth);

     sf.requestQuit();

     return debug_lines;
}