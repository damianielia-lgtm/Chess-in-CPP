#pragma once

#include <chrono>
#include <iostream>
#include <string>
#include <format>
#include <cstdint>

#include "../core/position.h"

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
        using namespace std::chrono;

        auto mins = duration_cast<:minutes>(duration);
        duration -= mins;
        auto secs = duration_cast<seconds>(duration);
        duration -= secs;
        auto ms = duration_cast<milliseconds>(duration);
        std::string formatted_time = std::format("{:02}:{:02}.{:03}", mins.count(), secs.count(), ms.count());
        header_ = "Calculating " + std::to_string(total_nodes_) + " nodes, expected duration " + formatted_time;
    }

    std::uint64_t total_nodes_;
    std::uint64_t completed_nodes_ = 0;
    std::string header_;
};

void print_board(const Position& position, const bool flip = false);
void print_pos_info(const Position& position);
