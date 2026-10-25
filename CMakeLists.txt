cmake_minimum_required(VERSION 3.20)

set(CMAKE_TOOLCHAIN_FILE "C:/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "" FORCE)

project(ChessInCpp LANGUAGES CXX)

include(CheckIPOSupported)

add_executable(MultiPLY
    src/main.cpp

    src/storage/file_manager.cpp
    src/storage/presets.cpp
    src/storage/config.cpp

    src/game/game_loop.cpp
    src/game/game.cpp

    src/notation/fen.cpp
    src/notation/move_notation.cpp
    src/notation/pgn_construction.cpp
    src/notation/pgn_parsing.cpp
    src/notation/pgn_reconstruction.cpp

    src/interface/display.cpp
    src/interface/cli_parser.cpp
    src/interface/cli_executor.cpp
    src/interface/uci.cpp

    src/movegen/legal_moves.cpp
    src/movegen/attacks.cpp

    src/engine/evaluation.cpp
    src/engine/search.cpp

    src/core/position.cpp
    
    src/diagnostics/perft.cpp
    src/diagnostics/stockfish_bridge.cpp
    src/diagnostics/debugger.cpp
    src/diagnostics/engine_benchmarking.cpp
    src/diagnostics/engine_profiling.cpp
)

add_custom_command(TARGET MultiPLY POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/resources"
        "$<TARGET_FILE_DIR:MultiPLY>/resources"
)

add_executable(picker_bench
    src/picker_lab/picker_microbenchmark.cpp
)

function(apply_project_standard target)
    check_ipo_supported(RESULT ipo_supported OUTPUT ipo_error)

    if(ipo_supported)
        set_property(TARGET ${target} PROPERTY INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)
    endif()

    set_target_properties(${target} PROPERTIES
        CXX_STANDARD 23
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
    )
    if(MSVC)
        target_compile_options(${target} PRIVATE
            /EHsc
            /W0
            /std:c++latest
            /permissive-
        )
    else()
        target_compile_options(${target} PRIVATE
            -Wall
            -Wextra
            -Wpedantic
        )
    endif()
endfunction()

apply_project_standard(MultiPLY)
apply_project_standard(picker_bench)

# Commands:
# cd "C:/Python projects/Chess in CPP"
# cmake --preset debug
# cmake --build --preset debug
# cmake --preset max
# cmake --build --preset max
