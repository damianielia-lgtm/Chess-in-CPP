#include <string>
#include <fstream>
#include <vector>
#include <sstream>
#include <iostream>
#include <map>
#include <chrono>
#include <format>
#include <cmath>
#include "..\app\core\encoding.h"
#include "..\app\game_logic\legal_moves.h"
#include "..\app\core\move_simulation.h"
#include "..\app\conversion\fen.h"
#include "perft.h"

uint64_t perft(Position& position, int depth) {
     if (depth == 0) {return 1;}
     MovesList legal_moves = all_legal_moves(position, false);
     if (depth == 1) {return legal_moves.count;}
     uint64_t count = 0;
     for (int index = 0; index < legal_moves.count; index++) {
          int move = legal_moves.moves[index];
          undo_storage move_state = apply_move(move, position);
          count += perft(position, depth - 1);
          take_back_move(move, position, move_state);
     }
     return count;
}

const std::map<std::string, uint64_t> preset_max_nodes = {
     {"Instant", 300000},
     {"Fast", 1000000},
     {"Moderate", 5000000},
     {"Extended", 20000000}
};

preset_info make_preset(std::string preset) {
     preset_info info;
     uint64_t max_nodes = preset_max_nodes.at(preset);
     for (expected_perft pos : epd_parser()) {
          std::map<int, uint64_t> per_depth_values;
          for (const auto [depth, expected] : pos.depths) {
               if ((expected <= max_nodes) && (depth > 0)) {
                    per_depth_values[depth] = expected;
                    info.total_nodes += expected;
                    if (expected >= 5000) {
                         info.engine_nodes += expected;
                    }
               }
          }
          if (!per_depth_values.empty()) {
               info.positions.push_back({pos.fen, pos.id, per_depth_values});
          }
     }
     return info;
}

std::vector<expected_perft> epd_parser() {
     std::vector<expected_perft> pos_list;
     std::ifstream file("diagnostics/perft_database.epd");
     std::string line;
     while (std::getline(file, line)) {
          expected_perft pos;

          std::vector<std::string> tokens;
          std::stringstream ss(line);
          std::string token;
          while (std::getline(ss, token, ';')) {
               if (token.empty()) {continue;}
               if (token[0] == ' ') {
                    token.erase(0, 1);
               }
               tokens.push_back(token);
          }

          pos.fen = tokens[0];

          for (size_t operator_index = 1; operator_index < tokens.size(); operator_index++) {
               std::string field = tokens[operator_index];
               if (field.starts_with("id")) {
                    pos.id = field.substr(3);
                    pos.id.erase(0, 1);
                    pos.id.pop_back();
               } else if (field.starts_with("D")) {
                    int depth = std::stoi(field.substr(1, field.find(' ') - 1));
                    uint64_t count = std::stoull(field.substr(field.find(' ') + 1));
                    pos.depths[depth] = count;
               }
          }

          pos_list.push_back(pos);
     }
     return pos_list;
}

std::string perft_test(Position& pos, uint64_t depth, uint64_t expected) {
     uint64_t nodes = perft(pos, depth);
     if (nodes == expected) {
          return "Depth " + std::to_string(depth) + ": " + std::to_string(nodes) + " [PASS]\n";
     } else {
          return "Depth " + std::to_string(depth) + ": " + std::to_string(nodes) + " [FAIL] (expected: " + std::to_string(expected) + ")\n";
     }
}

struct benchmark_tracking {
     std::string line;
     double duration;
     uint64_t nodes;
};

benchmark_tracking perft_benchmark(Position& pos, uint64_t depth, uint64_t expected) {
     if (expected <= 5000) {
          return {"Depth " + std::to_string(depth) + ": Value too low to calculate speed reliably.\n", 0, 0};
     }
     auto start = std::chrono::steady_clock::now();
     uint64_t nodes = perft(pos, depth);
     auto end = std::chrono::steady_clock::now();
     std::chrono::duration<double> dur = end - start; 
     double speed = std::round(nodes / dur.count() * 100.0) / 100.0;
     std::string line = "Depth " + std::to_string(depth) + ": ";
     line += std::to_string(nodes) + " nodes in " + std::format("{:.2f}", dur.count()) + "s - ";
     line += std::format("{}", speed) + " nodes/sec\n";
     return {line, dur.count(), nodes};
}

std::string run_perft(std::string preset, std::string mode) {
     preset_info test_info = make_preset(preset);

     Position test_pos = from_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
     auto start = std::chrono::steady_clock::now();
     perft(test_pos, 5);
     auto end = std::chrono::steady_clock::now();
     auto expected_duration = std::chrono::duration_cast<std::chrono::milliseconds>((end - start) * test_info.total_nodes / 4865609);

     auto mins = std::chrono::duration_cast<std::chrono::minutes>(expected_duration);
     expected_duration -= mins;
     auto secs = std::chrono::duration_cast<std::chrono::seconds>(expected_duration);
     expected_duration -= secs;
     auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(expected_duration);
     std::string formatted_time = std::format("{:02}:{:02}.{:03}", mins.count(), secs.count(), ms.count());

     std::string header_line = "Calculating " + std::to_string(test_info.total_nodes) + " nodes, expected duration " + formatted_time;
     std::cerr << header_line << "\n";

     std::string lines;
     uint64_t completed_nodes = 0;
     int percentage = 0;
     std::cerr << "\r[>                    ] 0%" << std::flush;

     lines += "----- Perft " + mode + " -----\n\n";
     double total_dur = 0.0;
     uint64_t total_nodes = 0;
     for (expected_perft perft_state : test_info.positions) {
          lines += "\n--- Running " + perft_state.id + " - Fen: '" + perft_state.fen + "' ---\n\n";
          Position pos = from_fen(perft_state.fen);
          
          for (const auto [depth, expected] : perft_state.depths) {
               if (mode == "Test") {
                    lines += perft_test(pos, depth, expected);
               } else if (mode == "Benchmark") {
                    benchmark_tracking benchmark_info = perft_benchmark(pos, depth, expected);
                    lines += benchmark_info.line;
                    total_dur += benchmark_info.duration;
                    total_nodes += benchmark_info.nodes;
               }
               
               completed_nodes += expected;
               percentage = completed_nodes * 100 / test_info.total_nodes;

               std::cerr << "\r[";
               int bar_len = percentage / 5;
               for (int i = 0; i < 20; i++) {
                    if (i < bar_len) std::cerr << "=";
                    else if (i == bar_len) std::cerr << ">";
                    else std::cerr << " ";
               }
               std::cerr << "] " << percentage << "%" << std::flush;
          }
     }
     
     std::string clear_bar(35, ' ');
     std::string clear_header(header_line.length(), ' ');
     std::cerr << "\r" << clear_bar; 
     std::cerr << "\033[A\r" << clear_header << "\r" << std::flush;

     lines += '\n';
     if (mode == "Benchmark") {
          std::string total = "Total: " + std::to_string(total_nodes) + " nodes in " + std::format("{:.2f}", total_dur);
          total += "s - Average Speed: " + std::format("{:.2f}", total_nodes / total_dur) + " nodes/s";
          lines += total;
     }
     return lines;
}