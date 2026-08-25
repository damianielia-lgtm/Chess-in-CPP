#pragma once

#include <optional>

#include "game.h"

std::optional<GameState> play_local(std::optional<TimeControl> time_control);
