#include "config_management.h"

#include <vector>
#include <string>
#include <filesystem>
#include <fstream>
#include <cctype>
#include <cstddef>

#include "../config.h"
#include "../errors.h"
#include "file_manager.h"

namespace fs = std::filesystem;

inline const fs::path config_dir{"data/config.cfg"};

std::vector<std::string> construct_config_save_lines(const ConfigData& config) {
    return {
        "player_1=" + config.white_name,
        "player_2=" + config.black_name,
        "event=" + config.event,
        "site=" + config.site,
        "export_clocks=" + (config.pgn_save_clock ? std::string("true") : std::string("false")),
        "move_input=" + (config.move_input == MoveInput::Uci ? std::string("uci") : std::string("san")),
        "board_orientation=" + (config.board_orientation == BoardOrientation::White ? std::string("white") : std::string("black"))
    };
}

void initialize_config() try {
    if (!fs::exists(config_dir)) {
        write_file(config_dir, construct_config_save_lines(ConfigData{}));
    }
} catch (const fs::filesystem_error& e) {
    throw StorageIoError("Filesystem error: " + std::string{e.what()});
}

void update_saved_config(const ConfigData& config) try {
    std::ofstream output{config_dir};

    if (!output) {
        throw StorageIoError("Could not open config file for updating.");
    }

    for (const std::string& line : construct_config_save_lines(config)) {
        output << line << '\n';
    }

    if (!output) {
        throw StorageIoError("Failed while writing to config file.");
    }
} catch (const fs::filesystem_error& e) {
    throw StorageIoError("Filesystem error: " + std::string{e.what()});
}

ConfigData load_saved_config() {
    ConfigData data;

    bool found_player_1 = false;
    bool found_player_2 = false;
    bool found_event = false;
    bool found_site = false;
    bool found_export_clocks = false;
    bool found_move_input = false;
    bool found_board_orientation = false;

    for (const std::string& line : read_file(config_dir)) {
        if (line.empty()) { continue; }
        
        size_t split_pos = line.find('=');

        if (split_pos == std::string::npos) {
            throw ConfigError("Invalid config line.");
        }

        std::string field = line.substr(0, split_pos);
        std::string value = line.substr(split_pos + 1);

        if (field == "player_1") {
            if (found_player_1) {
                throw ConfigError("Repeated player_1 configuration.");
            } else {
                found_player_1 = true;
                data.white_name = value;
            }
        } else if (field == "player_2") {
            if (found_player_2) {
                throw ConfigError("Repeated player_2 configuration.");
            } else {
                found_player_2 = true;
                data.black_name = value;
            }
        } else if (field == "event") {
            if (found_event) {
                throw ConfigError("Repeated event configuration.");
            } else {
                found_event = true;
                data.event = value;
            }
        } else if (field == "site") {
            if (found_site) {
                throw ConfigError("Repeated site configuration.");
            } else {
                found_site = true;
                data.site = value;
            }
        } else if (field == "export_clocks") {
            if (found_export_clocks) {
                throw ConfigError("Repeated export_clocks configuration.");
            } else {
                found_export_clocks = true;
                if (value == "true") {
                    data.pgn_save_clock = true;
                } else if (value == "false") {
                    data.pgn_save_clock = false;
                } else {
                    throw ConfigError("Unrecognized export_clocks config value.");
                }
            }
        } else if (field == "move_input") {
            if (found_move_input) {
                throw ConfigError("Repeated move_input configuration.");
            } else {
                found_move_input = true;
                if (value == "uci") {
                    data.move_input = MoveInput::Uci;
                } else if (value == "san") {
                    data.move_input = MoveInput::San;
                } else {
                    throw ConfigError("Unrecognized move_input config value.");
                }
            }
        } else if (field == "board_orientation") {
            if (found_board_orientation) {
                throw ConfigError("Repeated board_orientation configuration.");
            } else {
                found_board_orientation = true;
                if (value == "white") {
                    data.board_orientation = BoardOrientation::White;
                } else if (value == "black") {
                    data.board_orientation = BoardOrientation::Black;
                } else {
                    throw ConfigError("Unrecognized board_orientation config value.");
                }
            }
        } else {
            throw ConfigError("Unrecognized configuration field.");
        }
    }

    return data;
}
