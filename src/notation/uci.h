#pragma once

#include <string_view>

#include "../core/move.h"
#include "../core/position.h"

Move resolve_uci(Position& position, const std::string_view uci);
