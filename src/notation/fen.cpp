#include <string>
#include <sstream>
#include <array>
#include <cctype>
#include <stdexcept>

#include "../core/square.h"
#include "../core/position.h"
#include "../core/piece.h"
#include "../movegen/attacks.h"

struct FenFields {
    std::string board;
    std::string turn;
    std::string castling_rights;
    std::string en_passant_target;
    std::string halfmove_clock;
    std::string fullmove_number;
};

Position::Position(const std::string_view fen_string) {
    set_startpos();
    
    if (fen_string == "startpos") {
        set_startpos();
        return;
    }

    if (std::count(fen_string.begin(), fen_string.end(), ' ') != 5) {
        throw std::invalid_argument("FEN must have six fields");
    }
    std::stringstream ss{std::string(fen_string)};
    std::array<std::string, 6> collector;
    for (int i = 0; i < 6; ++i) {
        ss >> collector[i];
    }
    FenFields fen_info;
    fen_info.board = collector[0];
    fen_info.turn = collector[1];
    fen_info.castling_rights = collector[2];
    fen_info.en_passant_target = collector[3];
    fen_info.halfmove_clock = collector[4];
    fen_info.fullmove_number = collector[5];

    if (std::count(fen_info.board.begin(), fen_info.board.end(), '/') != 7 || fen_info.board.starts_with('/') || fen_info.board.ends_with('/')) {
        throw std::invalid_argument("Invalid board ranks");
    }
    clear_board();
    {
        int rank_square_count = 0;
        int fen_index = 0;
        int white_kings = 0;
        int black_kings = 0;
        Square square;
        for (std::size_t token_index = 0; token_index < fen_info.board.length(); token_index++) {
            char token = static_cast<unsigned char>(fen_info.board[token_index]);

            if (token == '/') {
                if (rank_square_count != 8) {
                        throw std::invalid_argument("Rank must have 8 squares");
                } else {
                        rank_square_count = 0;
                        continue;
                }
            } else if (rank_square_count >= 8) {
                throw std::invalid_argument("Rank must have 8 squares");
            }
            
            if (std::isdigit(token)) {
                if (std::isdigit(fen_info.board[token_index + 1])) {
                        throw std::invalid_argument("Consecutive digits in board");
                }

                if (token == '0' || token == '9') {
                        throw std::invalid_argument("Invalid board piece");
                }

                for (int i = 0; i < (token - '0'); i++) {
                        square = Square(static_cast<std::uint8_t>(fen_index ^ 56)); // Reverses rank to fit our indexing
                        set_piece(square, Piece());
                        fen_index++;
                        rank_square_count++;
                }

            } else if (std::string("pnbrqkPNBRQK").find(token) != std::string::npos) {
                square = Square(static_cast<std::uint8_t>(fen_index ^ 56)); // Reverses rank to fit our indexing

                set_piece(square, Piece(token));
                
                if (token == 'K') {white_kings++;}
                if (token == 'k') {black_kings++;}
                
                if ((token == 'P' || token == 'p') && (square.rank() == 0 || square.rank() == 7)) {
                        throw std::invalid_argument("Pawn on invalid rank");
                }
                
                fen_index++;
                rank_square_count++;
            } else {
                throw std::invalid_argument("Invalid board piece");
            }
        }
        if (white_kings != 1 || black_kings != 1) {
            throw std::invalid_argument("Invalid king count");
        }
        if (rank_square_count != 8) {
            throw std::invalid_argument("Rank must have 8 squares");
        }
    }

    if (fen_info.turn != "w" && fen_info.turn != "b") {
        throw std::invalid_argument("Invalid side to move");
    }
    turn_ = (fen_info.turn == "w") ? Color::White : Color::Black;

    castling_rights_.revoke_all();
    if (fen_info.castling_rights != "-") {
        std::string valid_castling = "KQkq";
        for (char c : fen_info.castling_rights) {
            auto pos = valid_castling.find(c);
            if (pos == std::string::npos) {
                throw std::invalid_argument("Invalid castling rights");
            } else {
                CastlingOption candidate_option(c);

                switch(candidate_option.color()) {
                        case Color::White:
                            if (piece_at(Square('e', '1')) != Piece(Color::White, PieceType::King)) {
                                throw std::invalid_argument("Missing white king for castling");
                            } break;
                        case Color::Black:
                            if (piece_at(Square('e', '8')) != Piece(Color::Black, PieceType::King)) {
                                throw std::invalid_argument("Missing black king for castling");
                            } break;
                }

                switch (candidate_option.encoding()) {
                        case CastlingType::WhiteKingside:
                            if (piece_at(Square('h', '1')) != Piece(Color::White, PieceType::Rook)) {
                                throw std::invalid_argument("Missing white rook for castling");
                            } break;
                        case CastlingType::WhiteQueenside: 
                            if (piece_at(Square('a', '1')) != Piece(Color::White, PieceType::Rook)) {
                                throw std::invalid_argument("Missing white rook for castling");
                            } break;
                        case CastlingType::BlackKingside:
                            if (piece_at(Square('h', '8')) != Piece(Color::Black, PieceType::Rook)) {
                                throw std::invalid_argument("Missing black rook for castling");
                            } break;
                        case CastlingType::BlackQueenside:
                            if (piece_at(Square('a', '8')) != Piece(Color::Black, PieceType::Rook)) {
                                throw std::invalid_argument("Missing black rook for castling");
                            } break;
                }
                
                castling_rights_.grant(candidate_option);
                valid_castling.erase(pos, 1);
            }
        }
    }

    if (fen_info.en_passant_target == "-") {
        en_passant_target_ = Square();
    } else {
        if (fen_info.en_passant_target.length() != 2) {
            throw std::invalid_argument("Invalid en passant square");
        }

        if (fen_info.en_passant_target[0] < 'a' || fen_info.en_passant_target[0] > 'h') {
            throw std::invalid_argument("Invalid en passant file");
        }
        if (fen_info.en_passant_target[1] != '3' && turn_ == Color::Black) {
            throw std::invalid_argument("Invalid en passant rank");
        }
        if (fen_info.en_passant_target[1] != '6' && turn_ == Color::White) {
            throw std::invalid_argument("Invalid en passant rank");
        }

        en_passant_target_ = Square(fen_info.en_passant_target[0], fen_info.en_passant_target[1]);
        
        if (!piece_at(en_passant_target_).empty()) {
            throw std::invalid_argument("En passant square occupied");
        }
        if (turn_ == Color::White) {
            if (piece_at(Square(en_passant_target_.index() - 8)) != Piece(Color::Black, PieceType::Pawn)) {
                throw std::invalid_argument("Missing en passant pawn");
            }
        } else {
            if (piece_at(Square(en_passant_target_.index() + 8)) != Piece(Color::White, PieceType::Pawn)) {
                throw std::invalid_argument("Missing en passant pawn");
            }
        }
    }

    if (fen_info.halfmove_clock.length() > 6 || fen_info.halfmove_clock.empty()) {
        throw std::invalid_argument("Invalid halfmove clock");
    } else {
        for (unsigned char c : fen_info.halfmove_clock) {
            if (!std::isdigit(c)) { throw std::invalid_argument("Invalid halfmove clock"); }
        }
    }
    halfmove_clock_ = std::stoi(fen_info.halfmove_clock);

    if (fen_info.fullmove_number.length() > 6 || fen_info.fullmove_number.empty() || fen_info.fullmove_number == "0") {
        throw std::invalid_argument("Invalid move clock");
    } else {
        for (unsigned char c : fen_info.fullmove_number) {
            if (!std::isdigit(c)) { throw std::invalid_argument("Invalid halfmove clock"); }
        }
    }
    fullmove_number_ = std::stoi(fen_info.fullmove_number);

    for (const Square square : Square::all()) {
        if (piece_at(square) == Piece(Color::White, PieceType::King)) {
            white_king_ = square;
        } else if (piece_at(square) == Piece(Color::Black, PieceType::King)) {
            black_king_ = square;
        }
    }

    if (is_attacked_square(*this, king_square(opposite_turn()), turn_)) {
        throw std::invalid_argument("CastlingSide not to move is in check");
    }
}

std::string Position::to_fen() const {
    std::string fen_string;

    std::string fen_board;
    int file_index = 0;
    for (const Square fen_square : Square::all()) {
        Square square = Square(fen_square.index() ^ 56); // Reverses rank to fit fen indexing
        if (piece_at(square) != Piece()) {
            fen_board += piece_at(square).symbol();
        } else {
            if (!fen_board.empty() && std::isdigit(fen_board.back())) {
                char increment = ++fen_board.back();
                fen_board.pop_back();
                fen_board += increment;
            } else {
                fen_board += '1';
            }
        }
        file_index++;
        if (file_index == 8) {
            file_index = 0;
            fen_board += '/';
        }
    }
    fen_board.pop_back();
    fen_string += fen_board + ' ';

    fen_string += (turn_ == Color::White) ? "w " : "b ";

    if (castling_rights_.encoding() == 0) {
        fen_string += '-';
    } else {
        for (const CastlingOption index : CastlingOption::all()) {
            if (castling_rights_.has(index)) { fen_string += index.fen_char(); }
        }
    }
    fen_string += ' ';

    if (en_passant_target_ == Square()) {
        fen_string += '-';
    } else {
        fen_string += en_passant_target_.uci_file();
        fen_string += en_passant_target_.uci_rank();
    }
    fen_string += ' ';

    fen_string += std::to_string(halfmove_clock_) + ' ';
    fen_string += std::to_string(fullmove_number_);

    return fen_string;
}
