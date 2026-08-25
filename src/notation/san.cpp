#include "san.h"

#include <string>
#include <string_view>
#include <vector>
#include <algorithm>

#include "../core/move.h"
#include "../core/piece.h"
#include "../core/position.h"
#include "../core/castling_rights.h"
#include "../movegen/legal_moves.h"
#include "../movegen/attacks.h"
#include "../errors.h"

std::string to_san(const Position& position, const Move move) {
    Position modifiable_copy = position;

    Square target = move.target();
    Square origin = move.origin();
    Piece moving_piece = modifiable_copy.piece_at(origin);
    MovesList legal_moves = all_moves(modifiable_copy, MoveGeneration::All);

    std::string piece_field;
    if (moving_piece.type() != PieceType::Pawn) {
        piece_field = Piece(Color::White, moving_piece.type()).symbol();
    } else if (move.is_capture()) {
        piece_field = origin.uci_file();
    }

    std::string disambiguation_field;
    if (
        moving_piece.type() != PieceType::Pawn &&
        moving_piece.type() != PieceType::King
    ) {
        std::vector<Square> ambiguous_squares;
        for (const Move& candidate : legal_moves) {
            if (
                candidate.origin() != origin && 
                candidate.target() == target &&
                modifiable_copy.piece_at(candidate.origin()) == moving_piece
            ) {
                ambiguous_squares.push_back(candidate.origin());
            }
        }

        if (ambiguous_squares.size() >= 1) {
            bool matching_file = false;
            bool matching_rank = false;
            for (const Square ambiguous_square : ambiguous_squares) {
                if (ambiguous_square.file() == origin.file()) { matching_file = true; }
                if (ambiguous_square.rank() == origin.rank()) { matching_rank = true; }
            }

            if (!matching_file) {
                disambiguation_field += origin.uci_file();
            } else if (!matching_rank) {
                disambiguation_field += origin.uci_rank();
            } else {
                disambiguation_field += origin.uci_file();
                disambiguation_field += origin.uci_rank();
            }
        }
    }

    std::string capture_field;
    if (move.is_capture()) {
        capture_field = 'x';
    }

    std::string destination_field;
    destination_field += target.uci_file();
    destination_field += target.uci_rank();

    std::string promotion_field;
    if (move.is_promotion()) {
        promotion_field = "=";
        promotion_field += Piece(Color::White, move.promotion_type()).symbol();
    }

    std::string check_or_mate_field;
    UndoState move_state = modifiable_copy.apply_move(move);
    bool in_check = is_attacked_square(
        modifiable_copy,
        modifiable_copy.king_square(modifiable_copy.turn()),
        modifiable_copy.opposite_turn()
    );
    if (in_check) {
        check_or_mate_field = (all_moves(modifiable_copy, MoveGeneration::All).empty())
            ? '#'
            : '+';
    }
    modifiable_copy.revert_move(move, move_state);


    if (move.is_castling()) {
        std::string castling_san;
        CastlingOption castling_type(move);
        if (castling_type.side() == CastlingSide::Kingside) {
            castling_san += "O-O";
        } else {
            castling_san += "O-O-O";
        }
        castling_san += check_or_mate_field;
        return castling_san;
    }

    std::string san;
    san += piece_field;
    san += disambiguation_field;
    san += capture_field;
    san += destination_field;
    san += promotion_field;
    san += check_or_mate_field;
    return san;
}

Move resolve_san(const Position& position, const std::string_view san) {
    Position modifiable_copy = position;
    for (const Move move : all_moves(modifiable_copy, MoveGeneration::All)) {
        if (to_san(position, move) == san) {
            return move;
        }
    }

    throw IllegalMoveError(std::string(san) + " is not a legal move on the current position.");
}
