#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_set>
#include <optional>

#include "../notation/pgn.h"
#include "../notation/move_notation.h"
#include "../errors.h"

struct OpeningData {
    std::string ECO_tag;
    std::vector<std::string> uci_moves;
};

std::optional<OpeningData> compile_opening(
    const std::vector<std::string>& pgn_lines,
    std::unordered_set<std::string>& unique_positions
) {
    ParsedPGN pgn_data = parse_pgn_document(pgn_lines);
    if (!pgn_data.ECO_tag) { return std::nullopt; }

    std::vector<std::string> uci_moves;

    Game game(
        pgn_data.white_name,
        pgn_data.black_name,
        pgn_data.event,
        pgn_data.site,
        std::nullopt,
        pgn_data.starting_position
    );

    for (const ParsedPly& ply : pgn_data.plies) {
        try {
            Move move = resolve_move(ply.san_move, game.live_position(), MoveNotation::San);
            uci_moves.push_back(move_notation(move, game.live_position(), MoveNotation::Uci));
            game.play_move(move);
        } catch (const IllegalMoveError& e) {
            return std::nullopt;
        }
    }

    if (!unique_positions.insert(game.live_position().to_fen(true)).second) {
        return std::nullopt;
    }

    return OpeningData{*pgn_data.ECO_tag, uci_moves};
}

void compile_book(
    std::string book_name,
    std::string compiled_name,
    std::string eval_var,
    int openings_count,
    int id_offset
) {
    std::ifstream pgn("resources/openings/" + book_name);
    std::vector<std::string> single_opening;
    std::string line;
    bool first_newline = true;
    int openings_inspected = 0;

    std::unordered_set<std::string> unique_positions;
    int id_number = id_offset * 1000000;

    std::cout << "Compiling " << book_name << "...\n"
        << "0/" << std::to_string(openings_count) << " openings...";

    std::vector<std::string> compiled_lines;

    while (std::getline(pgn, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        if (line.empty() && !single_opening.empty()) {
            if (first_newline) {
                single_opening.push_back(line);
                first_newline = false;
            } else {                
                std::optional<OpeningData> opening_data =
                    compile_opening(single_opening, unique_positions);

                if (opening_data) {
                    id_number++;

                    std::string compiled_line = std::to_string(id_number)
                        + "   " + opening_data.value().ECO_tag + "   ";
                    std::string last_move = opening_data.value().uci_moves.back();
                    opening_data.value().uci_moves.pop_back();
                    for (const std::string& move : opening_data.value().uci_moves) {
                        compiled_line += move + ' ';
                    }
                    compiled_line += last_move;
                    compiled_lines.push_back(compiled_line);
                }
                
                openings_inspected++;
                if (openings_inspected % 1000 == 0) {
                    std::cout << "\r\033[2K"
                        << std::to_string(openings_inspected) << "/"
                        << std::to_string(openings_count) << " openings...";
                }

                single_opening.clear();
                first_newline = true;
            }
        } else {
            single_opening.push_back(line);
        }
    }

    std::cout << "\r\033[2K"
        << std::to_string(openings_inspected) << "/"
        << std::to_string(openings_count) << " openings...\n";

    std::ofstream compiled_book{"resources/openings/" + compiled_name};
    compiled_book << "name=" << book_name << '\n'
        << "eval_var=" << eval_var << '\n'
        << "opening_count=" << std::to_string(compiled_lines.size()) << "\n\n"
        << "ID        ECO   UCI-MOVES\n";
    for (const std::string& compiled_line : compiled_lines) {
        compiled_book << compiled_line << '\n';
    }
}

int main() {
    compile_book("UHO_XXL_+0.80_+1.09.pgn", "very-low_var.txt", "+80/+109", 261028, 1);
    compile_book("UHO_XXL_+0.90_+1.19.pgn", "low_var.txt", "+90/+119", 223070, 2);
    compile_book("UHO_XXL_2022_+100_+129.pgn", "medium_var.txt", "+100/+129", 284035, 3);
    compile_book("UHO_XXL_2022_+110_+139.pgn", "high_var.txt", "+110/+139", 253847, 4);
    compile_book("UHO_XXL_2022_+120_+149.pgn", "very-high_var.txt", "+120/+149", 226629, 5);
    return 0;
}
