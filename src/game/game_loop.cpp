#include <string>
#include <iostream>
#include <chrono>
#include <optional>

#include "../core/position.h"
#include "../interface/display.h"
#include "../notation/uci.h"
#include "../errors.h"
#include "game.h"

std::optional<Game> play_local(std::optional<TimeControl> time_control) {
    using namespace std::chrono;

    std::size_t cursor = 0;
    Game game("White", "Black", time_control);
    GameDisplay display;

    display.render(game.snapshot_at(cursor));
    
    while (true) {
        display.clear_error();

        std::string user_input;
        auto start = steady_clock::now();
        if(!std::getline(std::cin, user_input)) { return std::nullopt; }
        auto end = steady_clock::now();
        milliseconds time_taken = duration_cast<milliseconds>(end - start);

        if (game.is_timed_game()) {
            game.consume_time(time_taken);
            game.check_game_end();
            if (game.has_ended()) { break; }
        }
        
        if (user_input == "next") {
            if (cursor < game.snapshot_count() - 1) { cursor++; }
        } else if (user_input == "previous") {
            if (cursor > 0) { cursor--; }
        } else if (user_input == "first") {
            cursor = 0;
        } else if (user_input == "last") {
            cursor = game.snapshot_count() - 1;
        } else if (cursor != game.snapshot_count() - 1) {
            display.set_error("Go to the most recent position to play moves.");
        } else {
            if (user_input == "resign") {
                game.resign();
            } else if (user_input == "draw") {
                game.agree_draw();
            } else {
                try {
                    game.play_move(
                        resolve_uci(game.live_position(), user_input)
                    );
                    game.check_game_end();
                    cursor++;
                } catch (const IllegalMoveError& e) {
                    display.set_error(e.what());
                }
            }

            if (game.has_ended()) { break; }
        }

        if (cursor == game.snapshot_count() - 1) {
            display.update(game.live_snapshot());
        } else {
            display.update(game.snapshot_at(cursor));
        }
    }

    display.set_result(game.result());
    display.update(game.live_snapshot());

    return game;
}

void replay(const Game& game) {
    std::size_t cursor = 0;

    GameDisplay display;
    if (cursor == game.snapshot_count() - 1) { display.set_result(game.result()); }
    display.render(game.snapshot_at(cursor));

    while (true) {
        display.clear_error();
        display.clear_result();
        
        std::string user_input;
        if(!std::getline(std::cin, user_input)) { return; }

        if (user_input == "next") {
            if (cursor < game.snapshot_count() - 1) { cursor++; }
        } else if (user_input == "previous") {
            if (cursor > 0) { cursor--; }
        } else if (user_input == "first") {
            cursor = 0;
        } else if (user_input == "last") {
            cursor = game.snapshot_count() - 1;
        } else if (user_input == "exit") {
            break;
        } else {
            display.set_error("Invalid navigation command");
        }

        if (cursor == game.snapshot_count() - 1) { display.set_result(game.result()); }
        display.update(game.snapshot_at(cursor));
    }
}
