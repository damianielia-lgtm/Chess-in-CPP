#include "game_loop.h"

#include <chrono>
#include <cstddef>
#include <iostream>
#include <string>
#include <string_view>

#include "../engine/search.h"
#include "../errors.h"
#include "../interface/display.h"
#include "../notation/move_notation.h"

using namespace std::chrono;

namespace {

bool handle_view_command(
    std::string_view input,
    std::size_t& cursor,
    std::size_t count,
    ConfigData& config
) {
    if (input == "flip") {
        config.board_orientation = config.board_orientation == BoardOrientation::White
            ? BoardOrientation::Black : BoardOrientation::White;
    } else if (input == "next") {
        if (cursor < count - 1) { cursor++; }
    } else if (input == "previous") {
        if (cursor > 0) { --cursor; }
    } else if (input == "first") {
        cursor = 0;
    } else if (input == "last") {
        cursor = count - 1;
    } else {
        return false;
    }
    return true;
}

void refresh_display(
    GameDisplay& display,
    const Game& game,
    std::size_t cursor,
    const ConfigData& config
) {
    display.update(
        cursor == game.snapshot_count() - 1 ? game.live_snapshot() : game.snapshot_at(cursor),
        config.board_orientation
    );
    display.clear_message();
}

enum class GameInput { IllegalInput, MovePlayed, GameEndedWithoutMove };

GameInput try_play_move(
    std::string_view input,
    Game& game,
    MoveNotation notation
) {
    if (input == "resign") {
        game.resign();
        return GameInput::GameEndedWithoutMove;
    }
    if (input == "draw") {
        game.agree_draw();
        return GameInput::GameEndedWithoutMove;
    }
    try {
        game.play_move(resolve_move(input, game.live_position(), notation));
    } catch (const IllegalMoveError&) {
        return GameInput::IllegalInput;
    }
    game.check_game_end();
    return GameInput::MovePlayed;
}

void handle_move_input(
    std::string_view input,
    Game& game,
    GameDisplay& display,
    ConfigData& config,
    std::size_t& cursor
) {
    display.add_newline_offset();
    if (handle_view_command(input, cursor, game.snapshot_count(), config)) { return; }

    if (cursor != game.snapshot_count() - 1) {
        display.set_error("Go to the most recent position to play moves.");
    } else if (try_play_move(input, game, config.move_notation) == GameInput::IllegalInput) {
        display.set_error(std::string(input) + " isn't legal.");
    } else if (!game.has_ended()) {
        cursor++;
    }
}

bool charge_elapsed_time(Game& game, steady_clock::time_point start) {
    const auto elapsed = duration_cast<milliseconds>(steady_clock::now() - start);
    if (game.is_timed_game()) {
        game.consume_time(elapsed);
        game.check_game_end();
    }
    return game.has_ended();
}

// A missing player color means both sides are controlled locally.
std::optional<Game> play_game(Game game, ConfigData& config, std::optional<Color> player_color) {
    std::size_t cursor = 0;
    GameDisplay display(game.metadata());

    while (!game.has_ended()) {
        refresh_display(display, game, cursor, config);

        if (player_color && game.live_position().turn() != *player_color) {
            Position position = game.live_position();
            Move move;

            if (game.is_timed_game()) {
                const auto start = steady_clock::now();
                move = *pbm_timed_game(
                    position,
                    game.live_snapshot().clock(position.turn()),
                    game.metadata().time_control->increment,
                    std::nullopt
                ).move;
                if (charge_elapsed_time(game, start)) { break; }
            } else {
                move = *pbm_fixed_time(position, milliseconds(3000)).move;
            }

            game.play_move(move);
            game.check_game_end();
            if (!game.has_ended()) { cursor++; }
        } else {
            std::string input;
            const auto start = steady_clock::now();
            if (!std::getline(std::cin, input)) { return std::nullopt; }
            if (charge_elapsed_time(game, start)) { break; }
            handle_move_input(input, game, display, config, cursor);
        }
    }

    display.set_result(game.result());
    display.update(game.live_snapshot(), config.board_orientation);
    return game;
}

}

std::optional<Game> play_local(std::optional<TimeControl> time_control, ConfigData& config) {
    return play_game(
        Game(config.player1_name, config.player2_name, config.event, config.site, time_control),
        config, std::nullopt
    );
}

std::optional<Game> play_engine(std::optional<TimeControl> time_control, Color player_color, ConfigData& config) {
    return play_game(
        Game(
            player_color == Color::White ? config.player1_name : "Engine",
            player_color == Color::White ? "Engine" : config.player1_name,
            config.event, config.site, time_control
        ),
        config, player_color
    );
}

std::optional<Game> analyze(const Position& position, ConfigData& config, bool clear_output_at_end) {
    std::size_t cursor = 0;
    Game game("White", "Black", config.event, config.site, std::nullopt, position);
    GameDisplay display(game.metadata());

    while (true) {
        refresh_display(display, game, cursor, config);
        std::string input;
        if (!std::getline(std::cin, input)) { return std::nullopt; }
        display.add_newline_offset();

        if (input == "exit") {
            if (!game.has_ended()) { game.finish_without_result(); }
            break;
        }
        if (handle_view_command(input, cursor, game.snapshot_count(), config)) { continue; }

        if (input == "fen") {
            display.set_message("Fen: \"" + game.live_position().to_fen() + '\"');
            continue;
        }

        if (input == "bestmove") {
            Position position = game.live_position();
            std::optional<Move> bestmove = pbm_fixed_time(position, milliseconds(3000)).move;
            std::string message = bestmove
                ? "Bestmove: " + move_notation(*bestmove, position, config.move_notation)
                : "No moves available.";
            display.set_message(message);
            continue;
        } else if (input == "eval") {
            Position position = game.live_position();
            std::int16_t eval = pbm_fixed_time(position, milliseconds(3000)).eval;
            display.set_message("Eval: " + std::to_string(eval));
            continue;
        }

        while (cursor < game.snapshot_count() - 1) { game.pop_state(); }
        const GameInput result = try_play_move(input, game, config.move_notation);
        if (result == GameInput::IllegalInput) {
            display.set_error(input + " isn't legal.");
            continue;
        }
        if (game.has_ended()) { display.set_result(game.result()); }
        if (result == GameInput::MovePlayed) { cursor++; }
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
        display.clear_message();
        display.clear_result();

        std::string input;
        if (!std::getline(std::cin, input)) { return; }
        display.add_newline_offset();
        if (handle_view_command(input, cursor, game.snapshot_count(), config)) { continue; }

        if (input == "analyze") {
            display.clear_rendered_area();
            analyze(game.snapshot_at(cursor).position(), config, true);
        } else if (input == "exit") {
            break;
        } else {
            display.set_error("Unrecognized command");
        }
    }
}
