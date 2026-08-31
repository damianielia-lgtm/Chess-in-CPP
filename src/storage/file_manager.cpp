#include "file_manager.h"

#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <vector>
#include <cctype>
#include <algorithm>

#include "../errors.h"

namespace fs = std::filesystem;

inline const std::filesystem::path pgn_dir{"data/pgn/"};
inline const std::filesystem::path fen_dir{"data/fen/"};
inline const std::filesystem::path report_dir{"data/reports/"};

void initialize_directories() try {
    fs::create_directories(pgn_dir);
    fs::create_directories(fen_dir);
    fs::create_directories(report_dir);
} catch (const fs::filesystem_error& e) {
    throw StorageError("Filesystem error: " + std::string{e.what()});
}

namespace {

void validate_name(const std::string_view name) {
    if (name.empty()) { throw StorageError("Name can't be empty."); }

    for (unsigned char c : name) {
        if (std::isalnum(c)) { continue; }
        if (c == '-' || c == '_') { continue; }
        throw StorageError("Invalid name. You can only use letters, digits, underscores, and dashes.");
    }
}

}

fs::path make_pgn_path(const std::string_view name) {
    validate_name(name);
    fs::path dir = pgn_dir / name;
    dir.replace_extension(".pgn");    
    return dir;
}

fs::path make_fen_path(const std::string_view name) {
    validate_name(name);
    fs::path dir = fen_dir / name;
    dir.replace_extension(".fen");    
    return dir;
}

fs::path make_report_path(const std::string_view name) {
    validate_name(name);
    fs::path dir = report_dir / name;
    dir.replace_extension(".txt");    
    return dir;
}

void delete_file(const fs::path& path) try {
    if (!fs::exists(path)) {
        throw StorageError(path.string() + " not found.");
    }

    fs::remove(path);
} catch (const fs::filesystem_error& e) {
    throw StorageIoError("Filesystem error: " + std::string{e.what()});
}

void write_file(const fs::path& path, const std::vector<std::string>& contents) try {
    if (fs::exists(path)) {
        throw StorageError(path.string() + " already exists.");
    }

    std::ofstream output{path};

    if (!output) {
        throw StorageIoError("Could not open " + path.string() + " for writing.");
    }

    for (const std::string& line : contents) {
        output << line << '\n';
    }

    if (!output) {
        throw StorageIoError("Failed while writing to " + path.string() + ".");
    }
} catch (const fs::filesystem_error& e) {
    throw StorageIoError("Filesystem error: " + std::string{e.what()});
}

std::vector<std::string> read_file(const fs::path& path) try {
    if (!fs::exists(path)) {
        throw StorageError(path.string() + " not found.");
    }

    std::ifstream input{path};
    if (!input) {
        throw StorageIoError("Could not open " + path.string() + " for reading.");
    }

    std::string line;
    std::vector<std::string> lines;

    while (std::getline(input, line)) {
        lines.push_back(line);
    }

    if (input.bad()) {
        throw StorageIoError("Failed while reading from " + path.string() + ".");
    }

    return lines;
} catch (const fs::filesystem_error& e) {
    throw StorageIoError("Filesystem error: " + std::string{e.what()});
}

std::vector<std::string> pgn_list() try {
    std::vector<std::string> files;
    for (const auto& entry : fs::directory_iterator{pgn_dir}) {
        if (
            entry.is_regular_file() &&
            entry.path().extension() == ".pgn"
        ) {
            files.push_back(entry.path().stem().string());
        }
    }

    std::sort(files.begin(), files.end());
    return files;
} catch (const fs::filesystem_error& e) {
    throw StorageIoError("Filesystem error: " + std::string{e.what()});
}
