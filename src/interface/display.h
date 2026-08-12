#pragma once

#include <chrono>
#include <iostream>
#include <string>
#include <vector>
#include <optional>
#include <cstdint>

#include "../core/position.h"
#include "../application/game.h"
#include "output_construction.h"

class PerftProgress {
public:
    PerftProgress(std::uint64_t total_nodes, std::chrono::milliseconds expected_duration)
        : total_nodes_(total_nodes)
    {
        make_header(expected_duration);
        std::cerr << header_ << '\n';
        print(0);
    }

    ~PerftProgress() {
        std::string clear_bar(35, ' ');
        std::string clear_header(header_.length(), ' ');
        std::cerr << "\r" << clear_bar;
        std::cerr << "\033[A\r" << clear_header << "\r" << std::flush;
    }

    void advance(std::uint64_t nodes) {
        completed_nodes_ += nodes;
        print(completed_nodes_);
    }

private:
    void print(std::uint64_t completed) {
        int percentage = static_cast<int>(completed * 100 / total_nodes_);
        int bar_len = percentage / 5;

        std::cerr << "\r[";
        for (int i = 0; i < 20; i++) {
            if (i < bar_len) { std::cerr << "="; }
            else if (i == bar_len) { std::cerr << ">"; }
            else { std::cerr << " "; }
        }
        std::cerr << "] " << percentage << "%" << std::flush;
    }

    void make_header(std::chrono::milliseconds duration) {
        header_ =
            "Calculating " + std::to_string(total_nodes_)
            + " nodes, expected duration " + format_time(duration);
    }

    std::uint64_t total_nodes_;
    std::uint64_t completed_nodes_ = 0;
    std::string header_;
};

class GameDisplay {
public:
    GameDisplay(bool is_timed) :
        show_clock_(is_timed),
        rendered_line_count_(0),
        error_message_(std::nullopt) {}

    void render(const GameState& game) {
        std::vector<std::string> lines = construct_game_lines(game, show_clock_, error_message_);
        rendered_line_count_ = lines.size();

        std::string prompt_line = lines.back();
        lines.pop_back();
        for (const std::string& line : lines) {
            std::cout << line << '\n';
        }

        std::cout << prompt_line;
    }

    void update(const GameState& game) {
        for (int __ = 0; __ < rendered_line_count_; __++) { // Clear line by line, going up
            std::cout << "\r"; // Go to the start of line
            std::cout << "\033[2K"; // Clear line
            std::cout << "\033[A"; // Go up one line
        }

        render(game);
    }

    void set_error(std::string message) { error_message_ = message; }
    void clear_error() { error_message_ = std::nullopt; }

private:
    bool show_clock_;
    int rendered_line_count_ = 0;
    std::optional<std::string> error_message_;
};

void print_pos_info(const Position& position);
