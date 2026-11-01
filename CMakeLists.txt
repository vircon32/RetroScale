# Minimum version of CMake that can parse this file
cmake_minimum_required(VERSION 3.16)

# Declare the project using C++ language
project(RetroScale LANGUAGES CXX)

# Find libpng along with everything it needs
find_package(PNG REQUIRED)

# define our final executable, and compile it with C++11
add_executable(retroscale
    Colors.cpp
    PaletteFileIO.cpp
    PNGFileIO.cpp
    RetroScale.cpp)
    
target_compile_features(retroscale PRIVATE cxx_std_11)

# link libpng to our program
target_link_libraries(retroscale PRIVATE PNG::PNG)

# extra compile options
if(MSVC)
  target_compile_options(retroscale PRIVATE /W4)
else()
  target_compile_options(retroscale PRIVATE -Wall -Wextra -Wpedantic)
endif()
