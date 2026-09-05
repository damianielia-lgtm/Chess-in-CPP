#include <string>
#include <iostream>
#include <chrono>
#include <optional>
#include <cstddef>

#include "../core/position.h"
#include "../interface/display.h"
#include "../notation/uci.h"
#include "../notation/san.h"
#include "../errors.h"
#include "../config.h"
#include "game.h"

std::optional<Game> play_local(std::optional<TimeControl> time_control, ConfigData& config) {
    using namespace std::chrono;

    std::size_t cursor = 0;
    Game game(config.white_name, config.black_name, config.event, config.site, time_control);
    GameDisplay display(game.metadata());

    display.render(game.snapshot_at(cursor), config.board_orientation);
    
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
        
        if (user_input == "flip") {
            config.board_orientation == BoardOrientation::White
                ? config.board_orientation = BoardOrientation::Black
                : config.board_orientation = BoardOrientation::White;
        }

        else if (user_input == "next") {
            if (cursor < game.snapshot_count() - 1) { cursor++; }
        } else if (user_input == "previous") {
            if (cursor > 0) { cursor--; }
        } else if (user_input == "first") {
            cursor = 0;
        } else if (user_input == "last") {
            cursor = game.snapshot_count() - 1;
        } else if (cursor != game.snapshot_count() - 1) {
            display.set_error("Go to the most recent position to play moves.");
        }
        
        else {
            if (user_input == "resign") {
                game.resign();
            } else if (user_input == "draw") {
                game.agree_draw();
            } else {
                try {
                    game.play_move(
                        config.move_input == MoveInput::Uci
                            ? resolve_uci(game.live_position(), user_input)
                            : resolve_san(game.live_position(), user_input)
                    );
                    game.check_game_end();
                    cursor++;
                } catch (const IllegalMoveError& e) {
                    display.set_error(e.what());
                }
            }

            if (game.has_ended()) { break; }
        }

        display.update(
            cursor == game.snapshot_count() - 1
                ? game.live_snapshot()
                : game.snapshot_at(cursor),
            config.board_orientation
        );
    }

    display.set_result(game.result());
    display.update(game.live_snapshot(), config.board_orientation);

    return game;
}

std::optional<Game> analyze(const Position& position, ConfigData& config, bool clear_output_at_end) {
    std::size_t cursor = 0;
    Game game("White", "Black", config.event, config.site, std::nullopt, position);
    GameDisplay display(game.metadata());

    display.render(game.snapshot_at(cursor), config.board_orientation);
    
    while (true) {
        display.clear_error();

        std::string user_input;
        if(!std::getline(std::cin, user_input)) { return std::nullopt; }

        if (user_input == "exit") {
            if (!game.has_ended()) { game.finish_without_result(); }
            break;
        }

        if (user_input == "flip") {
            config.board_orientation == BoardOrientation::White
                ? config.board_orientation = BoardOrientation::Black
                : config.board_orientation = BoardOrientation::White;
        }

        else if (user_input == "next") {
            if (cursor < game.snapshot_count() - 1) { cursor++; }
        } else if (user_input == "previous") {
            if (cursor > 0) { cursor--; }
        } else if (user_input == "first") {
            cursor = 0;
        } else if (user_input == "last") {
            cursor = game.snapshot_count() - 1;
        }
        
        else {
            while (cursor < game.snapshot_count() - 1) {
                game.pop_state();
            }
            
            if (user_input == "resign") {
                game.resign();
            } else if (user_input == "draw") {
                game.agree_draw();
            } else {
                try {
                    game.play_move(
                        config.move_input == MoveInput::Uci
                            ? resolve_uci(game.live_position(), user_input)
                            : resolve_san(game.live_position(), user_input)
                    );
                    game.check_game_end();
                    cursor++;
                } catch (const IllegalMoveError& e) {
                    display.set_error(e.what());
                }
            }

            if (game.has_ended()) { display.set_result(game.result());; }
        }

        display.update(
            cursor == game.snapshot_count() - 1
                ? game.live_snapshot()
                : game.snapshot_at(cursor),
            config.board_orientation
        );
    }

    if (clear_output_at_end) {
        display.clear_rendered_area();
    }

    return game;
}

void replay(const Game& game, ConfigData& config) {
    std::size_t cursor = 0;

    GameDisplay display(game.metadata());
    if (cursor == game.snapshot_count() - 1) { display.set_result(game.result()); }
    display.render(game.snapshot_at(cursor), config.board_orientation);

    while (true) {
        display.clear_error();
        display.clear_result();
        
        std::string user_input;
        if(!std::getline(std::cin, user_input)) { return; }

        if (user_input == "flip") {
            config.board_orientation == BoardOrientation::White
                ? config.board_orientation = BoardOrientation::Black
                : config.board_orientation = BoardOrientation::White;
        }

        else if (user_input == "analyze") {
            display.clear_rendered_area();
            analyze(game.snapshot_at(cursor).position(), config, true);
        }

        else if (user_input == "next") {
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
        display.update(game.snapshot_at(cursor), config.board_orientation);
    }
}
