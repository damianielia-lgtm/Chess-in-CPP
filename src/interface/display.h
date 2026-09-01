#pragma once

#include <chrono>
#include <iostream>
#include <string>
#include <vector>
#include <optional>
#include <cstdint>

#include "../core/position.h"
#include "../game/game.h"
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
    GameDisplay(GameMetadata metadata) :
        rendered_line_count_(0),
        error_message_(std::nullopt),
        result_(std::nullopt),
        metatdata_(std::move(metadata)),
        flip_board_(false) {}

    void render(const GameSnapshot& game){
        std::vector<std::string> lines = construct_game_lines(
            game,
            error_message_,
            result_,
            metatdata_,
            flip_board_
        );
        rendered_line_count_ = lines.size();

        std::string prompt_line = lines.back();
        lines.pop_back();
        for (const std::string& line : lines) {
            std::cout << line << '\n';
        }

        std::cout << prompt_line;
    }

    void update(const GameSnapshot& game) {
        for (int i = 0; i < rendered_line_count_; i++) { // Clear line by line, going up
            std::cout << "\r"; // Go to the start of line
            std::cout << "\033[2K"; // Clear line
            std::cout << "\033[A"; // Go up one line
        }

        render(game);
    }

    void set_error(std::string message) { error_message_ = message; }
    void clear_error() { error_message_ = std::nullopt; }

    void set_result(GameResult result) { result_ = result; }
    void clear_result() { result_ = std::nullopt; }

    void flip_board() { flip_board_ = !flip_board_; }

    void clear_rendered_area() {
        for (int i = 0; i < rendered_line_count_; i++) {
            std::cout << "\r";
            std::cout << "\033[2K";
            std::cout << "\033[A";
        }

        rendered_line_count_ = 0;
    }

private:
    std::size_t rendered_line_count_ = 0;
    std::optional<std::string> error_message_;
    std::optional<GameResult> result_;
    GameMetadata metatdata_;
    bool flip_board_;
};

void print_help();

void print_lines(const std::vector<std::string>& lines);
