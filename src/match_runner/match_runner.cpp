#include <iostream>
#include <fstream>
#include <string>
#include <chrono>
#include <optional>
#include <string_view>
#include <variant>
#include <random>
#include <algorithm>
#include <sstream>
#include <filesystem>
#include <format>
#include <cmath>

#include "../notation/move_notation.h"
#include "../notation/pgn.h"
#include "../game/game.h"
#include "../storage/process.h"

using namespace std::chrono;

/* --- INTERFACE --- */

struct CurrentConfig {
    std::optional<std::string> engine1_exe;
    std::optional<std::string> engine2_exe;
    std::optional<TimeControl> time_control;
    std::optional<int> n_games;
    std::optional<std::string> opening_book;
    std::optional<std::uint64_t> seed;
};

struct HelpCommand {};
struct ShowConfigCommand {};
struct RunCommand {};

struct SetEngine1Command { std::string exe; };
struct SetEngine2Command { std::string exe; };
struct SetTimeControlCommand { TimeControl time_control; };
struct SetNGamesCommand { int number; };
struct SetOpeningBookCommand { std::string book; };
struct SetSeedCommand { std::uint64_t seed; };

using Command = std::variant<
    HelpCommand,
    ShowConfigCommand,
    RunCommand,

    SetEngine1Command,
    SetEngine2Command,
    SetTimeControlCommand,
    SetNGamesCommand,
    SetOpeningBookCommand,
    SetSeedCommand
>;


/* --- PARSING --- */

std::uint64_t parse_number_string(std::string string) {
    if (string.empty()) { throw std::runtime_error("Invalid integer."); }
    if (string.length() > 20) { throw std::runtime_error("Integer too large."); }
    for (unsigned char c : string) {
        if (!std::isdigit(c)) { throw std::runtime_error("Invalid integer."); }
    }
    return std::stoull(string);
}

Command parse_command(const std::string_view line) {
    std::string token;
    std::vector<std::string> tokens;
    for (unsigned char c : line) {
        if (std::isspace(c)) {
            if (!token.empty()) {
                tokens.push_back(token);
                token.clear();
            }
            continue;
        }
        token += c;
    }
    if (!token.empty()) {
        tokens.push_back(token);
    }

    if (tokens.size() < 1) {
        throw std::runtime_error("Unrecognized command.");
    }
    const std::string& base_command = tokens[0];
    
    if (base_command == "help") {
        return HelpCommand{};
    } else if (base_command == "show-config") {
        if (tokens.size() != 1) {
            throw std::runtime_error("Malformed show-config command.");
        }
        return ShowConfigCommand{};
    } else if (base_command == "run") {
        if (tokens.size() != 1) {
            throw std::runtime_error("Malformed run command.");
        }
        return RunCommand{};
    } else if (base_command == "set") {
        if (tokens.size() != 3) {
            throw std::runtime_error("Malformed set command.");
        }

        const std::string set_field = tokens[1];
        const std::string set_value = tokens[2];

        if (set_field == "engine1") {
            return SetEngine1Command{set_value};
        } else if (set_field == "engine2") {
            return SetEngine2Command{set_value};
        } else if (set_field == "time-control") {
            auto first = set_value.find('+');
            auto last = set_value.rfind('+');

            if (first == std::string::npos || first != last) {
                throw std::runtime_error("Invalid time string.");
            }

            std::string initial{set_value.substr(0, first)};
            std::string increment{set_value.substr(first + 1)};

            TimeControl time_control{
                milliseconds{parse_number_string(initial)},
                milliseconds{parse_number_string(increment)}
            };
            return SetTimeControlCommand{time_control};
        } else if (set_field == "game-number") {
            std::uint64_t n = parse_number_string(set_value);
            if (n % 2 == 1 || 2 > n || n > 1000000) {
                throw std::runtime_error("Invalid game number.");
            }

            return SetNGamesCommand{int(n)};
        } else if (set_field == "opening-eval-variation") {
            if (
                set_value != "very-low" &&
                set_value != "low" &&
                set_value != "medium" &&
                set_value != "high" &&
                set_value != "very-high"
            ) {
                throw std::runtime_error("Unrecognized opening-eval-varaition.");
            }

            return SetOpeningBookCommand{set_value + "_var.txt"};
        } else if (set_field == "seed") {
            return SetSeedCommand{parse_number_string(set_value)};
        } else {
            throw std::runtime_error("Unrecognized set value.");
        }
    } else {
        throw std::runtime_error("Unrecognized command.");
    }
}


/* --- MATCH RUNNER --- */

struct Opening {
    int id;
    std::string ECO_tag;
    std::vector<std::string> uci_moves;
};

struct BookData {
    std::string book_name;
    std::string eval_var;
    std::vector<Opening> openings;
};

BookData read_opening_book(
    const std::string& opening_book,
    int openings_to_select,
    std::uint64_t seed
) {
    BookData data;

    std::ifstream book("resources/openings/" + opening_book);
    std::string line;

    std::getline(book, line);
    if (!line.starts_with("name=")) {
        throw std::runtime_error("Unexpected opening book.");
    }
    data.book_name = line.substr(5);

    std::getline(book, line);
    if (!line.starts_with("eval_var=")) {
        throw std::runtime_error("Unexpected opening book.");
    }
    data.eval_var = line.substr(9);

    std::getline(book, line);
    std::getline(book, line);
    std::getline(book, line);
    
    // Algorithm R selection
    std::vector<std::string> reservoir;
    reservoir.reserve(openings_to_select);
    std::mt19937_64 rng{seed};
    std::size_t i = 0; // lines seen so far

    while (std::getline(book, line)) {
        if (line.empty()) { continue; }

        if (i < openings_to_select) {
            reservoir.push_back(line); // fill the reservoir first
        } else {
            std::uniform_int_distribution<std::size_t> dist(0, i);
            std::size_t j = dist(rng); // keep this line with prob m/(i+1)
            if (j < openings_to_select) { reservoir[j] = line; } // ...replacing a random slot
        }
        i++;
    }

    for (const std::string& opening_line : reservoir) {
        size_t pos2 = opening_line.rfind("   ");
        if (pos2 == std::string::npos) {
            throw std::runtime_error("Unexpected opening book: Missing UCI separator.");
        }
        size_t pos1 = (pos2 >= 3) ? opening_line.rfind("   ", pos2 - 3) : std::string::npos;
        if (pos1 == std::string::npos) {
            throw std::runtime_error("Unexpected opening book: Missing ECO separator.");
        }
        std::string uci = opening_line.substr(pos2 + 3);
        std::string ECO_tag = opening_line.substr(pos1 + 3, pos2 - pos1 - 3);
        std::string id = opening_line.substr(0, pos1);

        std::vector<std::string> uci_moves;
        std::istringstream stream(uci);
        std::string move;
        while (stream >> move) {
            uci_moves.push_back(move);
        }

        data.openings.push_back({int(parse_number_string(id)), ECO_tag, uci_moves});
    }

    return data;
}

std::string get_engine_name(Process& pipe) {
    std::string engine_name;
    pipe.send_line("uci");
    for (const std::string& id_line : pipe.read_until("uciok")) {
        if (id_line.starts_with("id name ")) {
            engine_name = id_line.substr(8);
            break;
        }
    }
    if (engine_name.empty()) {
        throw std::runtime_error("Engine doesn't comply to uci.");
    }
    return engine_name;
}

bool play_engine_move(
    Process& engine,
    Game& game,
    std::vector<std::string>& moves_played
) {
    std::string position_line = "position startpos moves";
    for (const std::string& move : moves_played) {
        position_line += ' ' + move;
    }
    engine.send_line(position_line);

    const auto start = steady_clock::now();

    engine.send_line(
        "go wtime " + std::to_string(game.live_snapshot().clock(Color::White).count()) +
        " btime " + std::to_string(game.live_snapshot().clock(Color::Black).count()) +
        " winc " + std::to_string(game.metadata().time_control.value().increment.count()) +
        " binc " + std::to_string(game.metadata().time_control.value().increment.count())
    );
    std::string bestmove_line = engine.read_until("bestmove ").back().substr(9);

    const auto elapsed = duration_cast<milliseconds>(steady_clock::now() - start);
    game.consume_time(elapsed);
    game.check_game_end();
    if (game.has_ended()) { return true; }

    std::string bestmove;
    std::istringstream response(bestmove_line);
    if (!(response >> bestmove)) {
        throw std::runtime_error("Engine returned an empty bestmove.");
    }
    
    game.play_move(resolve_move(bestmove, game.live_position(), MoveNotation::Uci));
    moves_played.push_back(bestmove);
    game.check_game_end();
    if (game.has_ended()) { return true; }

    return false;
}

enum class Winner {
    White, Black, Draw
};

Winner run_and_save_game(
    TimeControl time_control,
    Process& white_engine,
    Process& black_engine,
    const std::string& white_engine_name,
    const std::string& black_engine_name,
    const std::string& event,
    int opening_id,
    const std::string& eco_tag,
    const std::vector<std::string>& opening_moves,
    const std::string& pgn_directory,
    int& game_number
) {
    white_engine.send_line("ucinewgame");
    black_engine.send_line("ucinewgame");

    Game game(
        white_engine_name,
        black_engine_name,
        event,
        "MultiPLY match runner",
        time_control,
        std::nullopt,
        opening_id,
        eco_tag
    );

    std::vector<std::string> moves;

    for (const std::string& opening_move : opening_moves) {
        game.consume_time(time_control.increment);
        game.play_move(resolve_move(opening_move, game.live_position(), MoveNotation::Uci));
        moves.push_back(opening_move);
    }

    while (true) {
        if (game.live_position().turn() == Color::White) {
            if (play_engine_move(white_engine, game, moves)) { break; }
        } else {
            if (play_engine_move(black_engine, game, moves)) { break; }
        }
    }

    game_number++;
    std::ofstream pgn{
        pgn_directory + "/" + std::format("{:05}_opening-{}.pgn", game_number, opening_id)
    };
    for (const std::string& pgn_line : construct_pgn_lines(game, true)) {
        pgn << pgn_line << '\n';
    }

    switch (game.result()) {
        case GameResult::White_by_Checkmate:
        case GameResult::White_by_Resignation:
        case GameResult::White_by_Timeout:
        case GameResult::White_by_Unknown:
            return Winner::White;
        case GameResult::Black_by_Checkmate:
        case GameResult::Black_by_Resignation:
        case GameResult::Black_by_Timeout:
        case GameResult::Black_by_Unknown:
            return Winner::Black;
        case GameResult::Draw_by_Agreement:
        case GameResult::Draw_by_FiftyMove:
        case GameResult::Draw_by_InsufficientMaterial:
        case GameResult::Draw_by_Stalemate:
        case GameResult::Draw_by_ThreefoldRepetition:
        case GameResult::Draw_by_Unknown:
            return Winner::Draw;
    }
}

struct MatchConfig {
    std::string engine1_exe;
    std::string engine2_exe;
    TimeControl time_control;
    int n_games;
    std::string opening_book;
    std::uint64_t seed;
};

void run_match(const MatchConfig& config) {
    Process engine1(config.engine1_exe);
    Process engine2(config.engine2_exe);

    std::string engine1_name = get_engine_name(engine1);
    std::string engine2_name = get_engine_name(engine2);
    std::string date = std::format("{:%d.%m.%Y-%H.%M.%S}", system_clock::now());
    std::string event = engine1_name + " v.s. " + engine2_name +  " --- " + date;
    std::string match_dir = "data/matches/" + event;
    std::filesystem::create_directories(match_dir);

    int game_number = 0;
    
    struct WDL {
        int wins = 0;
        int draws = 0;
        int losses = 0;
    };
    WDL total;
    WDL as_white;
    WDL as_black;

    BookData opening_data =
        read_opening_book(config.opening_book, config.n_games / 2, config.seed);

    std::cout << "Game 0/" << std::to_string(config.n_games) << "...";

    engine1.send_line("isready");
    engine1.read_until("readyok");
    engine2.send_line("isready");
    engine2.read_until("readyok");

    for (const Opening& opening : opening_data.openings) {
        switch (
            run_and_save_game(
                config.time_control,
                engine1, engine2,
                engine1_name, engine2_name,
                event, opening.id,
                opening.ECO_tag, opening.uci_moves,
                match_dir, game_number
            )
        ) {
            case Winner::White:
                total.wins++;
                as_white.wins++;
                break;
            case Winner::Black:
                total.losses++;
                as_white.losses++;
                break;
            case Winner::Draw:
                total.draws++;
                as_white.draws++;
        }
        std::cout << "\r\033[2K" << "Game " << std::to_string(game_number)
            << "/" << config.n_games << "...";

        switch (
            run_and_save_game(
                config.time_control,
                engine2, engine1,
                engine2_name, engine1_name,
                event, opening.id,
                opening.ECO_tag, opening.uci_moves,
                match_dir, game_number
            )
        ) {
            case Winner::White:
                total.losses++;
                as_black.losses++;
                break;
            case Winner::Black:
                total.wins++;
                as_black.wins++;
                break;
            case Winner::Draw:
                total.draws++;
                as_black.draws++;
        }
        std::cout << "\r\033[2K" << "Game " << std::to_string(game_number)
            << "/" << config.n_games << "...";
    }

    double score = static_cast<double>(total.wins + 0.5 * total.draws)
        / (total.wins + total.draws + total.losses);
    double elo_diff = 400 * std::log10(score / (1 - score));
    std::string summary_text = std::format(
        "--- {} v.s. {} ---\n\n"
        "Time Control (ms): {}+{}\n"
        "Opening book: {}\n"
        "Eval variation: {}\n"
        "Games: {} ({} paired openings)\n"
        "Opening pick seed: {}\n\n"
        "ELO diff:    {:.2f}\n"
        "Score:       {:.2f}%\n"
        "W / D / L:   {} / {} / {}\n"
        "As white:    {} / {} / {}\n"
        "As black:    {} / {} / {}\n",
        engine1_name, engine2_name,
        config.time_control.initial.count(), config.time_control.increment.count(),
        opening_data.book_name,
        opening_data.eval_var,
        config.n_games, config.n_games / 2,
        config.seed,
        elo_diff,
        score * 100.0,
        total.wins, total.draws, total.losses,
        as_white.wins, as_white.draws, as_white.losses,
        as_black.wins, as_black.draws, as_black.losses
    );

    std::cout << "\r\033[2K" << std::setprecision(2) << summary_text;
    
    std::ofstream summary{match_dir + "/summary.txt"};
    summary << std::setprecision(2) << summary_text;
}


/* --- EXECUTION --- */

void execute_impl(const HelpCommand&, CurrentConfig&) {
    std::cout << "Commands:\n"
        << " - set engine1 <first engine executable path>\n"
        << " - set engine2 <second engine exectuable path>\n"
        << " - set time-control <initial(ms)+increment(ms)>\n"
        << " - set game-number <even number of games>\n"
        << " - set opening-eval-variation { very-low | low | medium | high | very-high }\n"
        << " - set seed <seed number>\n"
        << " - show-config\n"
        << " - run\n";
}

void execute_impl(const ShowConfigCommand&, CurrentConfig& config) {
    std::cout
        << "Engine 1 executable:     "
            << (config.engine1_exe ? *config.engine1_exe : "(not set)")
            << "\n"
        << "Engine 2 executable:     "
            << (config.engine2_exe ? *config.engine2_exe : "(not set)")
            << "\n"
        << "Time control (ms):       "
            << (config.time_control
                ? std::to_string(config.time_control.value().initial.count()) + '+'
                    + std::to_string(config.time_control.value().increment.count())
                : "(not set)"
            ) << "\n"
        << "Number of games:         "
            << (config.n_games ? std::to_string(*config.n_games) : "(not set)")
            << "\n"
        << "Opening book:            "
            << (config.opening_book ? *config.opening_book : "(not set)")
            << "\n"
        << "Seed:                    "
            << (config.seed ? std::to_string(*config.seed) : "(not set)")
            << "\n";
}

void execute_impl(const RunCommand&, CurrentConfig& config) {
    if (
        !config.engine1_exe ||
        !config.engine2_exe ||
        !config.time_control ||
        !config.n_games ||
        !config.opening_book ||
        !config.seed
    ) {
        throw std::runtime_error("All configurations must be set before running match.");
    }

    run_match(
        MatchConfig{
            *config.engine1_exe,
            *config.engine2_exe,
            *config.time_control,
            *config.n_games,
            *config.opening_book,
            *config.seed
        }
    );
}

void execute_impl(const SetEngine1Command& cmd, CurrentConfig& config) { config.engine1_exe = cmd.exe; }
void execute_impl(const SetEngine2Command& cmd, CurrentConfig& config) { config.engine2_exe = cmd.exe; }
void execute_impl(const SetTimeControlCommand& cmd, CurrentConfig& config) { config.time_control = cmd.time_control; }
void execute_impl(const SetNGamesCommand& cmd, CurrentConfig& config) { config.n_games = cmd.number; }
void execute_impl(const SetOpeningBookCommand& cmd, CurrentConfig& config) { config.opening_book = cmd.book; }
void execute_impl(const SetSeedCommand& cmd, CurrentConfig& config) { config.seed = cmd.seed; }

void execute(const Command& command, CurrentConfig& config) {
    std::visit(
        [&config](const auto& concrete_command) {
            execute_impl(concrete_command, config);
        },
        command
    );
}


/* ------ */

int main() {
    std::filesystem::create_directories("data/matches/");

    CurrentConfig config(
        std::nullopt,
        std::nullopt,
        TimeControl{milliseconds(2000), milliseconds(0)},
        100,
        "medium_var.txt",
        1
    );

    while (true) {
        std::cout << "\033[32mmatch-runner> \033[0m";

        std::string line;
        if (!std::getline(std::cin, line)) { break; }

        if (line.empty()) { continue; }
        if (line == "quit") { break; }

        try {
            Command command = parse_command(line);
            execute(command, config);
        } catch (const std::exception& e) {
            std::cout << "\033[31m" << e.what() << "\033[0m\n";
            continue;
        }
    }

    return 0;
}
