#pragma once

#include <string>
#include <string_view>

#include "../core/move.h"
#include "../core/position.h"

std::string to_san(Position& position, const Move move);
Move resolve_san(Position& position, const std::string_view san);
