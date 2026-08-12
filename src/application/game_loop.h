#pragma once

#include <optional>

#include "game.h"

GameState play_local(std::optional<TimeControl> time_control);
