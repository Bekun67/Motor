cmake_minimum_required(VERSION 3.10)

project(Motor)

find_package(SDL3 CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)
find_package(assimp CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(DevIL REQUIRED)

file(GLOB SOURCES "src/*.cpp" "src/*.h")
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/src PREFIX "Source" FILES ${SOURCES})
add_executable(Motor ${SOURCES})

target_link_libraries(Motor PRIVATE SDL3::SDL3)
target_link_libraries(Motor PRIVATE glad::glad)
target_link_libraries(Motor PRIVATE assimp::assimp)
target_link_libraries(Motor PRIVATE glm::glm)
target_link_libraries(Motor PRIVATE DevIL::IL)
target_link_libraries(Motor PRIVATE imgui::imgui)