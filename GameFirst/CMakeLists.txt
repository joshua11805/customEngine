# Require CMake 3.31 or higher
cmake_minimum_required(VERSION 3.31)

project(Game)

# Build for C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable some specific compile flags on Windows/Mac
include(../cmake/SetCompileFlags.cmake)

# Add Engine library
add_subdirectory(Engine)
# Add Game executable
add_subdirectory(Game)
# Add Unit test
add_subdirectory(UnitTest)

