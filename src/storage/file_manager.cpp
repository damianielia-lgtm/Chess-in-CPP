#include "file_manager.h"

#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <vector>
#include <stdexcept>
#include <cctype>

void initialize_directories() {
    std::filesystem::create_directories(pgn_dir);
    std::filesystem::create_directories(fen_dir);
    std::filesystem::create_directories(report_dir);
}

void validate_name(const std::string_view name) {
    if (name.empty()) { throw std::invalid_argument("Name can't be empty."); }

    for (unsigned char c : name) {
        if (std::isalnum(c)) { continue; }
        if (c == '-' || c == '_') { continue; }
        throw std::invalid_argument("Invalid name. You can only use letters, digits, underscores, and dashes.");
    }
}

std::filesystem::path make_pgn_path(const std::string_view name) {
    validate_name(name);
    std::filesystem::path dir = pgn_dir / name;
    dir.replace_extension(".pgn");    
    return dir;
}

std::filesystem::path make_fen_path(const std::string_view name) {
    validate_name(name);
    std::filesystem::path dir = fen_dir / name;
    dir.replace_extension(".fen");    
    return dir;
}

std::filesystem::path make_report_path(const std::string_view name) {
    validate_name(name);
    std::filesystem::path dir = report_dir / name;
    dir.replace_extension(".txt");    
    return dir;
}

void delete_file(const std::filesystem::path& path) {
    if (!std::filesystem::exists(path)) {
        throw std::invalid_argument(path.string() + " not found.");
    }

    std::filesystem::remove(path);
}

void write_file(const std::filesystem::path& path, const std::vector<std::string>& contents) {
    if (std::filesystem::exists(path)) {
        throw std::invalid_argument(path.string() + " already exists.");
    }

    std::ofstream output{path};

    if (!output) {
        throw std::invalid_argument("Could not open " + path.string() + " for writing.");
    }

    for (const std::string& line : contents) {
        output << line << '\n';
    }

    if (!output) {
        throw std::invalid_argument("Failed while writing to " + path.string() + ".");
    }
}

std::vector<std::string> read_file(const std::filesystem::path& path) {
    if (!std::filesystem::exists(path)) {
        throw std::invalid_argument(path.string() + " not found.");
    }

    std::ifstream input{path};
    if (!input) {
        throw std::invalid_argument("Could not open " + path.string() + " for reading.");
    }

    std::string line;
    std::vector<std::string> lines;

    while (std::getline(input, line)) {
        lines.push_back(line);
    }

    if (input.bad()) {
        throw std::invalid_argument("Failed while reading from " + path.string() + ".");
    }

    return lines;
}

std::vector<std::string> pgn_list() {
    std::vector<std::string> files;
    for (const auto& entry : std::filesystem::directory_iterator{pgn_dir}) {
        if (
            entry.is_regular_file() &&
            entry.path().extension() == ".pgn"
        ) {
            files.push_back(entry.path().stem().string());
        }
    }

    return files;
}
