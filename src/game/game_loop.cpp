#include <string>
#include <iostream>
#include <chrono>
#include <optional>

#include "../core/position.h"
#include "../interface/display.h"
#include "../errors.h"
#include "game.h"

std::optional<GameState> play_local(std::optional<TimeControl> time_control) {
    using namespace std::chrono;

    GameState game(time_control);

    GameDisplay display(game.is_timed_game());
    display.render(game);
    
    while (true) {
        if (game.has_ended()) { break; }

        std::string user_input;
        auto start = steady_clock::now();
        if(!std::getline(std::cin, user_input)) { return std::nullopt; }
        auto end = steady_clock::now();
        milliseconds time_taken = duration_cast<milliseconds>(end - start);

        try {
            game.handle_user_input(user_input, time_taken);
            display.clear_error();
        } catch (const IllegalMoveError& e) {
            display.set_error(e.what());
        }
        
        display.update(game);
    }

    return game;
}
