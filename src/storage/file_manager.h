#pragma once

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

inline const std::filesystem::path pgn_dir{"data/pgn/"};
inline const std::filesystem::path fen_dir{"data/fen/"};
inline const std::filesystem::path report_dir{"data/reports/"};

void initialize_directories();

std::filesystem::path make_pgn_path(const std::string_view name);
std::filesystem::path make_fen_path(const std::string_view name);
std::filesystem::path make_report_path(const std::string_view name);

void delete_file(const std::filesystem::path& path);
void write_file(const std::filesystem::path& path, const std::vector<std::string>& contents);
std::vector<std::string> read_file(const std::filesystem::path& path);
std::vector<std::string> pgn_list();
