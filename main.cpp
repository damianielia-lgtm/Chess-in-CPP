#include <iostream>
#include <string>
#include <algorithm>
#include <cctype>
#include "diagnostics/perft.h"
#include "diagnostics/debugger.h"
#include "app/conversion/fen.h"

void pvp() {};
void pve(int player_color, int depth) {};
void replay_initialisation() {};
void analysis_initialisation() {};

int main() {
     std::string mode;
     while (true) {
          std::cout << "Would you like to play, replay, analyse a position, run perft, debug, or quit?";
          std::getline(std::cin, mode);
          if (mode != "replay" && mode != "play" && mode != "analyse position" && mode != "quit" && mode != "perft" && mode != "debug") {
               std::cout << "Answer with play, replay, analyse position, perft, debug or quit.\n";
               continue;
          } else {
               break;
          }
     }

     if (mode == "replay") {
          replay_initialisation();
     } else if (mode == "play") {
          std::string opponent;
          while (true) {
               std::cout << "Would you like to play locally, online or against the engine?";
               std::getline(std::cin, opponent);
               if (opponent != "local" && opponent != "online" && opponent != "engine") {
                    std::cout << "Plese answer with local, online or engine.\n";
                    continue;
               } else {
                    break;
               }
          }
          
          if (opponent == "local") {
               pvp();
          } else if (opponent == "online") {
               std::cout << "In works!";
          } else if (opponent == "engine") {
               std::string player_colour_str;
               while (true) {
                    std::cout << "Do you want the white or black pieces?";
                    std::getline(std::cin, player_colour_str);
                    if (player_colour_str != "white" && player_colour_str != "black") {
                         std::cout << "Please answer with white or black.\n";
                         continue;
                    } else {
                         break;
                    }
               }

               int player_color = (player_colour_str == "white") ? 0 : 1;
               std::string depth_str;
               while (true) {
                    std::cout << "What depth should the engine search at?";
                    std::getline(std::cin, depth_str);
                    if (!std::all_of(depth_str.begin(), depth_str.end(), ::isdigit) || (std::stoi(depth_str) <= 0)) {
                              std::cout << "The depth should be a positive integer.\n";
                              continue;
                         } else {
                              break;
                         }
               }

               int depth = std::stoi(depth_str);
               pve(player_color, depth);
          }
     } else if (mode == "analyse position") {
          analysis_initialisation();
     } else if (mode == "perft") {
          std::string perft_mode;
          while (true) {
               std::cout << "Do you want to run a benchmark or correctness test?";
               std::getline(std::cin, perft_mode);
               if (perft_mode != "Test" && perft_mode != "Benchmark") {
                    std::cout << "Please answer with 'Test' or 'Benchmark'.\n";
                    continue;
               } else {
                    break;
               }
          }
          std::string preset;
          while (true) {
               std::cout << "What speed preset should perft run at?";
               std::getline(std::cin, preset);
               if (preset != "Instant" && preset != "Fast" && preset != "Moderate" && preset != "Extended") {
                    std::cout << "Please answer with 'Insant', 'Fast', 'Moderate' or 'Extended'.\n";
                    continue;
               } else {
                    break;
               }
          }
          std::cout << run_perft(preset, perft_mode);
     } else if (mode == "debug") {
          std::string input_fen;
          while (true) {
               std::cout << "What position would you like to debug?";
               std::getline(std::cin, input_fen);
               if (!is_valid_fen(input_fen)) {
                    std::cout << "Please answer with a valid fen.\n";
                    continue;
               } else {
                    break;
               }
          }

          std::string depth_str;
          while (true) {
               std::cout << "At what depth?";
               std::getline(std::cin, depth_str);
               if (!std::all_of(depth_str.begin(), depth_str.end(), ::isdigit) || (std::stoi(depth_str) <= 0)) {
                         std::cout << "The depth should be a positive integer.\n";
                         continue;
                    } else {
                         break;
                    }
          }

          int depth = std::stoi(depth_str);
          std::cout << debug_pos(input_fen, depth);
     }

     return 0;
}