#include <string>
#include <iostream>
#include <stdexcept>
#include <thread>
#include <chrono>

#include "interface/cli_parser.h"
#include "interface/executor.h"
#include "interface/session.h"
#include "storage/file_manager.h"
#include "storage/config_management.h"
#include "config.h"
#include "errors.h"

int main() {
    try {
        initialize_directories();
        initialize_config();
    } catch (const StorageIoError& e) {
        std::cerr << "\033[31mFailed to initialize directories: " << e.what() << '\n';
        std::cerr << "Terminating program...\033[0m\n";
        std::this_thread::sleep_for(std::chrono::seconds(5));
        return 2;
    }

    Session session;
    ConfigData config;

    try {
        config = load_saved_config();
    } catch (const StorageIoError& e) {
        std::cerr << "\033[31mFailed to load configuration file: " << e.what() << '\n';
        std::cerr << "Terminating program...\033[0m\n";
        std::this_thread::sleep_for(std::chrono::seconds(5));
        return 2;
    } catch (const ConfigError& e) {
        std::cerr << "\033[31mInvalid configuration file: " << e.what() << '\n';
        std::cerr << "Delete the config file to regenerate the default configuration.\n";
        std::cerr << "Terminating program...\033[0m\n";
        std::this_thread::sleep_for(std::chrono::seconds(5));
        return 2;
    }

    while (true) {
        std::cout << "chess> ";
        std::string line;
        
        if (!std::getline(std::cin, line)) { break; }
        if (line.empty()) { continue; }
        if (line == "exit") { break; }

        try {
            Command command = parse(line);
            execute(command, session, config);
            update_saved_config(config);
        } catch (const UserError& e) {
            std::cerr << "\033[31m" << e.what() << "\033[0m\n";
            continue;
        } catch (const OperationalError& e) {
            std::cerr << "\033[31mOperational error: " << e.what() << '\n';
            continue;
        } catch (const std::exception& e) {
            std::cerr << "\033[31mUnexpected behavior: " << e.what() << '\n';
            std::cerr << "Terminating program...\033[0m\n";
            std::this_thread::sleep_for(std::chrono::seconds(5));
            return 1;
        }
    }

    return 0;
}
