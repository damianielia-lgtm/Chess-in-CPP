#include <string>
#include <iostream>
#include <stdexcept>

#include "interface/cli_parser.h"
#include "interface/executor.h"
#include "application/session.h"

int main() {
    Session session;

    while (true) {
        std::cout << "chess> ";
        std::string line;
        
        if(!std::getline(std::cin, line)) { break; }
        if (line.empty()) { continue; }
        if (line == "exit") { break; }

        try {
            Command command = parse(line);
            execute(command, session);
        } catch (const std::invalid_argument& e) {
            std::cerr << "\033[31m" << e.what() << "\033[0m\n";
            continue;
        }
    }

    return 0;
}
