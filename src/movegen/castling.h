#pragma once

#include "../core/position.h"
#include "../core/castling_rights.h"

bool can_castle(const Position& position, const CastlingOption castling_index);
