#pragma once

#include <vector>
#include <string>

#include "../config.h"

void initialize_config();

void update_saved_config(const ConfigData& config);

ConfigData load_saved_config();
