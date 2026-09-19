#pragma once

#include <chrono>
#include <iostream>
#include <string>
#include <vector>
#include <optional>
#include <cstdint>
#include <cstddef>

#include "../core/position.h"
#include "../game/game.h"
#include "../config.h"

void print_lines(const std::vector<std::string>& lines);

std::vector<std::string> help_lines();

std::vector<std::string> construct_board_lines(
    const Position& position,
    BoardOrientation board_orientation,
    std::optional<Move> move = std::nullopt
);

class ProgressDisplay {
public:
    ProgressDisplay(std::uint64_t total_nodes);
    ~ProgressDisplay();

    void advance(std::uint64_t nodes);

private:
    void print_progress(std::uint64_t completed);

    std::uint64_t total_nodes_;
    std::uint64_t completed_nodes_ = 0;
};

class GameDisplay {
public:
    GameDisplay(GameMetadata metadata) :
        rendered_line_count_(0),
        error_message_(std::nullopt),
        result_(std::nullopt),
        metatdata_(std::move(metadata)) {}

    void update(const GameSnapshot& game, BoardOrientation board_orientation);

    void set_error(std::string message) { error_message_ = message; }
    void clear_error() { error_message_ = std::nullopt; }

    void set_result(GameResult result) { result_ = result; }
    void clear_result() { result_ = std::nullopt; }

    void clear_rendered_area();

private:
    std::size_t rendered_line_count_ = 0;
    std::optional<std::string> error_message_;
    std::optional<GameResult> result_;
    GameMetadata metatdata_;
};
