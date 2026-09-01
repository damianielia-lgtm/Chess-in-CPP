#pragma once

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

void initialize_directories();

std::filesystem::path make_pgn_path(const std::string_view name);
std::filesystem::path make_fen_path(const std::string_view name);
std::filesystem::path make_report_path(const std::string_view name);

void delete_file(const std::filesystem::path& path);
void write_file(const std::filesystem::path& path, const std::vector<std::string>& contents);
std::vector<std::string> read_file(const std::filesystem::path& path);

std::vector<std::string> pgn_list();
std::vector<std::string> fen_list();
std::vector<std::string> report_list();
