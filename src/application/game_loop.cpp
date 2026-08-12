#include <string>
#include <iostream>
#include <chrono>
#include <optional>

#include "../core/position.h"
#include "../interface/display.h"
#include "game.h"

GameState play_local(std::optional<TimeControl> time_control) {
    using namespace std::chrono;

    GameState game(time_control);

    GameDisplay display(game.is_timed_game());
    display.render(game);
    
    while (true) {
        if (game.has_ended()) { break; }

        std::string uci_move;
        auto start = steady_clock::now();
        if(!std::getline(std::cin, uci_move)) { break; }
        auto end = steady_clock::now();
        milliseconds time_taken = duration_cast<milliseconds>(end - start);

        try {
            game.play_move(uci_move, time_taken);
            display.clear_error();
        } catch (const std::invalid_argument& e) {
            display.set_error(e.what());
        }
        
        display.update(game);
    }

    return game;
}
