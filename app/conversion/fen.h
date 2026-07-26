#pragma once
#include <string>
#include "../core/encoding.h"

Position from_fen(const std::string& fen_string);
std::string to_fen(const Position& position);
bool is_valid_fen(std::string fen_string);