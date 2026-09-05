#pragma once

#include <optional>

#include "../config.h"
#include "game.h"

std::optional<Game> play_local(std::optional<TimeControl> time_control, ConfigData& config);
void replay(const Game& game, ConfigData& config);
std::optional<Game> analyze(const Position& position, ConfigData& config, bool clear_output_at_end);
