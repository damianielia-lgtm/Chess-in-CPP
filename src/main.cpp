#include <string>
#include <iostream>
#include <stdexcept>
#include <thread>
#include <chrono>

#include "interface/cli_parser.h"
#include "interface/executor.h"
#include "interface/session.h"
#include "storage/file_manager.h"
#include "errors.h"

int main() {
    try {
        initialize_directories();
    } catch (const StorageIoError& e) {
        std::cerr << "\033[31mOperational error: "  << e.what() << '\n';
        std::cerr << "Terminating program...\033[0m\n";
        std::this_thread::sleep_for(std::chrono::seconds(5));
        return 2;
    }

    Session session;

    while (true) {
        std::cout << "chess> ";
        std::string line;
        
        if (!std::getline(std::cin, line)) { break; }
        if (line.empty()) { continue; }
        if (line == "exit") { break; }

        try {
            Command command = parse(line);
            execute(command, session);
        } catch (const UserError& e) {
            std::cerr << "\033[31m" << e.what() << "\033[0m\n";
            continue;
        } catch (const OperationalError& e) {
            std::cerr << "\033[31mOperational error: " << e.what() << '\n';
            std::cerr << "Terminating program...\033[0m\n";
            std::this_thread::sleep_for(std::chrono::seconds(5));
            return 2;
        } catch (const std::exception& e) {
            std::cerr << "\033[31mUnexpected behavior: " << e.what() << '\n';
            std::cerr << "Terminating program...\033[0m\n";
            std::this_thread::sleep_for(std::chrono::seconds(5));
            return 1;
        }
    }

    return 0;
}
