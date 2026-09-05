#pragma once

#include "../config.h"
#include "commands.h"
#include "session.h"

void execute(const Command& command, Session& session, ConfigData& config);
