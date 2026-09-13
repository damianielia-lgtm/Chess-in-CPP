#pragma once

#include <string>

#include "../config.h"
#include "commands.h"
#include "session.h"

Command parse(std::string line);
void execute(const Command& command, Session& session, ConfigData& config);
