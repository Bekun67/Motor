cmake_minimum_required(VERSION 3.10)

project(Motor)

find_package(SDL3 CONFIG REQUIRED)
find_package(assimp CONFIG REQUIRED)

add_executable(Engine src/Main.cpp)

target_link_libraries(Engine PRIVATE SDL3::SDL3)
target_link_libraries(Engine PRIVATE assimp::assimp)