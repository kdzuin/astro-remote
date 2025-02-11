cmake_minimum_required(VERSION 3.10)
project(m5remote_simulator)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set SDL2 paths explicitly for Mac
set(SDL2_DIR "/opt/homebrew/Cellar/sdl2/2.32.0/lib/cmake/SDL2")
set(SDL2_TTF_DIR "/opt/homebrew/Cellar/sdl2_ttf/2.24.0/lib/cmake/SDL2_ttf")

# Find SDL2
find_package(SDL2 REQUIRED)
find_package(SDL2_ttf REQUIRED)

# Add simulator source files
set(SIMULATOR_SOURCES
    src/compat/display.cpp
    src/compat/M5Unified.cpp
)

# Create simulator library
add_library(m5simulator STATIC ${SIMULATOR_SOURCES})

target_include_directories(m5simulator PUBLIC
    /opt/homebrew/include
    ${SDL2_INCLUDE_DIRS}
    ${SDL2_TTF_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/../src  # For M5Remote headers
)

target_link_libraries(m5simulator PUBLIC
    SDL2::SDL2
    SDL2::SDL2main
    SDL2_ttf::SDL2_ttf
)

# Add compile definition for simulator mode
target_compile_definitions(m5simulator PUBLIC SIMULATOR_MODE)

# Create simulator test executable
add_executable(simulator_test test/main.cpp)
target_link_libraries(simulator_test PRIVATE m5simulator)

# Create app test executable that uses real application code
add_executable(app_test test/app_test.cpp)
target_link_libraries(app_test PRIVATE m5simulator)

# Copy assets to build directory
add_custom_command(
    TARGET simulator_test POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/assets
    ${CMAKE_BINARY_DIR}/assets
)

add_custom_command(
    TARGET app_test POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/assets
    ${CMAKE_BINARY_DIR}/assets
)
