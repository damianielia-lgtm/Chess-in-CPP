#include "perft.h"

#include <string>
#include <fstream>
#include <vector>
#include <sstream>
#include <iostream>
#include <map>
#include <chrono>
#include <format>
#include <cmath>

#include "../movegen/legal_moves.h"
#include "../core/position.h"
#include "../core/move.h"
#include "../core/move_list.h"
#include "../interface/commands.h"

std::uint64_t perft_impl(
    Position& position,
    int depth,
    std::size_t ply,
    MoveListStack& move_lists
) {
    assert(ply < max_ply);

    if (depth == 0) { return 1; }
    
    MovesList& legal_moves = move_lists[ply];
    legal_moves.clear();
    generate_all_moves(legal_moves, position, false);
    
    if (depth == 1) { return legal_moves.size(); }

    std::uint64_t count = 0;
    for (const Move move : legal_moves) {
        UndoState move_state = position.apply_move(move);
        count += perft_impl(position, depth - 1, ply + 1, move_lists);
        position.revert_move(move, move_state);
    }

    return count;
}

std::uint64_t perft(Position& position, int depth) {
    MoveListStack move_lists;
    return perft_impl(position, depth, 0, move_lists);
}

struct ExpectedPerft {
    std::string fen;
    std::string id;
    std::map<int, uint64_t> depths;
};

std::vector<ExpectedPerft> epd_parser() {
    std::ifstream file("resources/diagnostics/perft_database.epd");
    if (!file) {
        throw std::runtime_error("Could not load perft database");
    }

    std::vector<ExpectedPerft> pos_list;
    std::string line;

    while (std::getline(file, line)) {
        ExpectedPerft pos;

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
                std::uint64_t count = std::stoull(field.substr(field.find(' ') + 1));
                pos.depths[depth] = count;
            }
        }

        pos_list.push_back(pos);
    }

    return pos_list;
}

struct PresetInfo {
    uint64_t total_nodes = 0;
    uint64_t engine_nodes = 0;
    std::vector<ExpectedPerft> positions{};
};

const std::map<std::string, std::uint64_t> preset_max_nodes = {
    {"instant", 300000},
    {"fast", 1000000},
    {"moderate", 5000000},
    {"extended", 20000000}
};

PresetInfo make_preset(Preset preset) {
    PresetInfo info;
    std::uint64_t max_nodes;
    switch (preset) {
        case Preset::Instant: max_nodes = preset_max_nodes.at("instant"); break;
        case Preset::Fast: max_nodes = preset_max_nodes.at("fast"); break;
        case Preset::Moderate: max_nodes = preset_max_nodes.at("moderate"); break;
        case Preset::Extended: max_nodes = preset_max_nodes.at("extended"); break;
        default: throw std::invalid_argument("Invalid preset");
    }

    for (ExpectedPerft pos : epd_parser()) {
        std::map<int, std::uint64_t> per_depth_values;

        for (const auto [depth, expected] : pos.depths) {
            if ((expected <= max_nodes) && (depth > 0)) {
                per_depth_values[depth] = expected;
                info.total_nodes += expected;

                if (expected >= 5000) { info.engine_nodes += expected; }
            }
        }

        if (!per_depth_values.empty()) {
            info.positions.push_back({pos.fen, pos.id, per_depth_values});
        }
    }

    return info;
}

std::string perft_test(Position& pos, int depth, std::uint64_t expected) {
    std::uint64_t nodes = perft(pos, depth);
    std::string perft_line;

    if (nodes == expected) {
        perft_line = "Depth " + std::to_string(depth) + ": " + std::to_string(nodes) + " [PASS]\n";
    } else {
        perft_line = "Depth " + std::to_string(depth) + ": " + std::to_string(nodes);
        perft_line += " [FAIL] (expected: " + std::to_string(expected) + ")\n";
    }

    return perft_line;
}

struct BenchmarkResult {
    std::string line;
    double duration;
    std::uint64_t nodes;
};

BenchmarkResult perft_benchmark(Position& pos, int depth, std::uint64_t expected) {
    if (expected <= 5000) {
        return {"Depth " + std::to_string(depth) + ": Value too low to calculate speed reliably.\n", 0, 0};
    }

    auto start = std::chrono::steady_clock::now();
    std::uint64_t nodes = perft(pos, depth);
    auto end = std::chrono::steady_clock::now();
    std::chrono::duration<double> dur = end - start; 
    double speed = std::round(nodes / dur.count() * 100.0) / 100.0;

    std::string line = "Depth " + std::to_string(depth) + ": ";
    line += std::to_string(nodes) + " nodes in " + std::format("{:.2f}", dur.count()) + "s - ";
    line += std::format("{}", speed) + " nodes/sec\n";
    return {line, dur.count(), nodes};
}

#include <iostream>

std::string run_perft(Preset preset, PerftMode mode) {
    PresetInfo test_info;
    test_info = make_preset(preset);

    Position test_pos("startpos");
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
    std::uint64_t completed_nodes = 0;
    int percentage = 0;
    std::cerr << "\r[>                    ] 0%" << std::flush;

    lines += "\n----- Perft -----\n\n";
    double total_dur = 0.0;
    std::uint64_t total_nodes = 0;
    for (ExpectedPerft perft_state : test_info.positions) {
        lines += "\n--- Running " + perft_state.id + " - Fen: '" + perft_state.fen + "' ---\n\n";
        Position pos(perft_state.fen);
        
        for (const auto [depth, expected] : perft_state.depths) {
            if (mode == PerftMode::Test) {
                lines += perft_test(pos, depth, expected);
            } else if (mode == PerftMode::Benchmark) {
                BenchmarkResult benchmark_info = perft_benchmark(pos, depth, expected);
                lines += benchmark_info.line;
                total_dur += benchmark_info.duration;
                total_nodes += benchmark_info.nodes;
            }
            
            completed_nodes += expected;
            percentage = static_cast<int>(completed_nodes * 100 / test_info.total_nodes);

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
    if (mode == PerftMode::Benchmark) {
        std::string total = "Total: " + std::to_string(total_nodes) + " nodes in " + std::format("{:.2f}", total_dur);
        total += "s - Average Speed: " + std::format("{:.2f}", total_nodes / total_dur) + " nodes/s\n";
        lines += total;
    }
    return lines;
}
