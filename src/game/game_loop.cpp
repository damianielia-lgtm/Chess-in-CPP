#include "game_loop.h"

#include <string>
#include <string_view>
#include <iostream>
#include <chrono>
#include <optional>
#include <cstddef>

#include "../core/position.h"
#include "../interface/display.h"
#include "../notation/move_notation.h"
#include "../engine/search.h"
#include "../errors.h"
#include "../config.h"
#include "game.h"

namespace {

void flip_orientation(ConfigData& config) {
    config.board_orientation = config.board_orientation == BoardOrientation::White
        ? BoardOrientation::Black
        : BoardOrientation::White;
}

bool handle_navigation(std::string_view input, std::size_t& cursor, std::size_t current_game_length) {
    if (input == "next") {
        if (cursor < current_game_length - 1) { cursor++; }
        return true;
    } else if (input == "previous") {
        if (cursor > 0) { cursor--; }
        return true;
    } else if (input == "first") {
        cursor = 0;
        return true;
    } else if (input == "last") {
        cursor = current_game_length - 1;
        return true;
    }

    return false;
}

void refresh_display(GameDisplay& display, const Game& game, std::size_t cursor, const ConfigData& config) {
    display.update(
        cursor == game.snapshot_count() - 1 ? game.live_snapshot() : game.snapshot_at(cursor),
        config.board_orientation
    );
    display.clear_error();
}

enum class GameInput {
    IllegalInput,
    MovePlayed,
    GameEndedWithoutMove
};

GameInput try_play_move(std::string_view user_input, Game& game, MoveNotation notation) {
    if (user_input == "resign") { game.resign(); return GameInput::GameEndedWithoutMove; }
    if (user_input == "draw") { game.agree_draw(); return GameInput::GameEndedWithoutMove; }

    try {
        game.play_move(resolve_move(user_input, game.live_position(), notation));
    } catch (const IllegalMoveError&) {
        return GameInput::IllegalInput;
    }

    game.check_game_end();
    return GameInput::MovePlayed;
}

enum class TurnOutcome { Continue, Break };

TurnOutcome handle_move_input(
    std::string_view user_input,
    Game& game,
    GameDisplay& display,
    ConfigData& config,
    std::size_t& cursor
) {
    if (user_input == "flip") { flip_orientation(config); return TurnOutcome::Continue; }
    if (handle_navigation(user_input, cursor, game.snapshot_count())) { return TurnOutcome::Continue; }

    if (cursor != game.snapshot_count() - 1) {
        display.set_error("Go to the most recent position to play moves.");
        return TurnOutcome::Continue;
    }

    if (try_play_move(user_input, game, config.move_notation) == GameInput::IllegalInput) {
        display.set_error(std::string(user_input) + " isn't legal.");
        return TurnOutcome::Continue;
    }

    if (game.has_ended()) { return TurnOutcome::Break; }
    cursor++;
    return TurnOutcome::Continue;
}

}

std::optional<Game> play_local(std::optional<TimeControl> time_control, ConfigData& config) {
    using namespace std::chrono;

    std::size_t cursor = 0;
    Game game(config.player1_name, config.player2_name, config.event, config.site, time_control);
    GameDisplay display(game.metadata());

    while (true) {
        refresh_display(display, game, cursor, config);

        std::string user_input;
        auto start = steady_clock::now();
        if (!std::getline(std::cin, user_input)) { return std::nullopt; }
        auto end = steady_clock::now();
        milliseconds time_taken = duration_cast<milliseconds>(end - start);

        if (game.is_timed_game()) {
            game.consume_time(time_taken);
            game.check_game_end();
            if (game.has_ended()) { break; }
        }

        if (handle_move_input(user_input, game, display, config, cursor) == TurnOutcome::Break) { break; }
    }

    display.set_result(game.result());
    display.update(game.live_snapshot(), config.board_orientation);

    return game;
}

std::optional<Game> play_engine(Color player_color, ConfigData& config) {
    std::size_t cursor = 0;
    Game game(
        player_color == Color::White ? config.player1_name : "Engine",
        player_color == Color::White ? "Engine" : config.player1_name,
        config.event,
        config.site
    );
    GameDisplay display(game.metadata());

    while (true) {
        refresh_display(display, game, cursor, config);

        if (game.live_position().turn() != player_color) {
            Position current_position = game.live_position();
            game.play_move(*pick_best_move(current_position, config.engine_depth).move);

            game.check_game_end();
            if (game.has_ended()) { break; }

            cursor++;
        } else {
            std::string user_input;
            if (!std::getline(std::cin, user_input)) { return std::nullopt; }

            if (handle_move_input(user_input, game, display, config, cursor) == TurnOutcome::Break) { break; }
        }
    }

    display.set_result(game.result());
    display.update(game.live_snapshot(), config.board_orientation);

    return game;
}

std::optional<Game> analyze(const Position& position, ConfigData& config, bool clear_output_at_end) {
    std::size_t cursor = 0;
    Game game("White", "Black", config.event, config.site, std::nullopt, position);
    GameDisplay display(game.metadata());

    while (true) {
        refresh_display(display, game, cursor, config);

        std::string user_input;
        if (!std::getline(std::cin, user_input)) { return std::nullopt; }

        if (user_input == "exit") {
            if (!game.has_ended()) { game.finish_without_result(); }
            break;
        }

        if (user_input == "flip") { flip_orientation(config); continue; }

        if (handle_navigation(user_input, cursor, game.snapshot_count())) { continue; }

        while (cursor < game.snapshot_count() - 1) { game.pop_state(); }

        GameInput input_result = try_play_move(user_input, game, config.move_notation);
        if (input_result == GameInput::IllegalInput) {
            display.set_error(user_input + " isn't legal.");
            continue;
        }

        if (game.has_ended()) { display.set_result(game.result()); }
        if (input_result == GameInput::MovePlayed) { cursor++; }
    }

    if (clear_output_at_end) { display.clear_rendered_area(); }

    return game;
}

void replay(const Game& game, ConfigData& config) {
    std::size_t cursor = 0;

    GameDisplay display(game.metadata());

    while (true) {
        if (cursor == game.snapshot_count() - 1) { display.set_result(game.result()); }
        display.update(game.snapshot_at(cursor), config.board_orientation);

        display.clear_error();
        display.clear_result();

        std::string user_input;
        if (!std::getline(std::cin, user_input)) { return; }

        if (user_input == "flip") { flip_orientation(config); continue; }

        if (handle_navigation(user_input, cursor, game.snapshot_count())) { continue; }

        if (user_input == "analyze") {
            display.clear_rendered_area();
            analyze(game.snapshot_at(cursor).position(), config, true);
            continue;
        }
        if (user_input == "exit") { break; }

        display.set_error("Unrecognized command");
    }
}
