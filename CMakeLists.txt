cmake_minimum_required(VERSION 3.16)

project(CharyRick LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED IMPORTED_TARGET sdl2)
pkg_check_modules(SDL2_TTF REQUIRED IMPORTED_TARGET SDL2_ttf)

add_executable(CharyRick
    src/ascii_art.c
    src/entity.c
    src/game.c
    src/input.c
    src/main.c
    src/renderer.c
    src/world.c
)

enable_testing()

add_test(NAME debug_snapshot
    COMMAND $<TARGET_FILE:CharyRick> --debug-snapshot --seed 12345 --floor 2
)

set_tests_properties(debug_snapshot PROPERTIES
    PASS_REGULAR_EXPRESSION "=== CHARYRICK DEBUG SNAPSHOT ===;window=;player=;00 \\|"
)

target_include_directories(CharyRick PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(CharyRick PRIVATE
    PkgConfig::SDL2
    PkgConfig::SDL2_TTF
    m
)
