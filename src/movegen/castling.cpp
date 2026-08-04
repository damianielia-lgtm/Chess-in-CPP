#include "castling.h"

#include "../core/position.h"
#include "../core/castling_rights.h"
#include "../core/square.h"
#include "attacks.h"
#include "move_tables.h"

constexpr std::array<Targets<3>, 4> CastlingEmpty = {{
    {Square('f', '1'), Square('g', '1')},
    {Square('b', '1'), Square('c', '1'), Square('d', '1')},
    {Square('f', '8'), Square('g', '8')},
    {Square('b', '8'), Square('c', '8'), Square('d', '8')}
}};
constexpr std::array<Targets<3>, 4> CastlingSafe = {{
    {Square('e', '1'), Square('f', '1'), Square('g', '1')},
    {Square('e', '1'), Square('d', '1'), Square('c', '1')},
    {Square('e', '8'), Square('f', '8'), Square('g', '8')},
    {Square('e', '8'), Square('d', '8'), Square('c', '8')}
}};

bool can_castle(const Position& position, const CastlingOption castling_index) {
    if (!position.castling_rights().has(castling_index)) { return false; }

    for (const Square empty_square : CastlingEmpty[castling_index.index()]) {
        if (!position.piece_at(empty_square).empty()) { return false; }
    }

    for (const Square safe_square : CastlingSafe[castling_index.index()]) {
        if (is_attacked_square(position, safe_square, position.opposite_turn())) { return false; }
    }
    
    return true;
}
