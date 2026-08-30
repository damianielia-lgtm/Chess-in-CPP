#pragma once

#include <optional>

#include "game.h"

std::optional<Game> play_local(std::optional<TimeControl> time_control);
void replay(const Game& game);
