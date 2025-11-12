cmake_minimum_required(VERSION 3.10)

project(Motor)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(SDL3 CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)
find_package(assimp CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(imguizmo CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(DevIL REQUIRED)

file(GLOB CORE_FILES 
    "src/Application.*" 
    "src/Main.*"
    "src/Window.*")

file(GLOB COMPONENT_FILES 
    "src/Component*.*")

file(GLOB MODULE_FILES 
    "src/Module*.*"
    "src/GameObject.*")

file(GLOB GRAPHICS_FILES 
    "src/OpenGL.*"
    "src/PrimitiveGenerator.*"
    "src/Texture.*"
    "src/Camera.*")

file(GLOB IO_FILES 
    "src/Input.*"
    "src/LoadFBX.*")

file (GLOB IMPORTER_FILES
    "src/MeshImporter.*"
    "src/TextureImporter.*"
    "src/FileSystemManager.*")

add_executable(Motor
    ${CORE_FILES}
    ${COMPONENT_FILES}
    ${MODULE_FILES}
    ${GRAPHICS_FILES}
    ${IO_FILES}
    ${IMPORTER_FILES}
)

source_group("Core" FILES ${CORE_FILES})
source_group("Components" FILES ${COMPONENT_FILES})
source_group("Modules" FILES ${MODULE_FILES})
source_group("Graphics" FILES ${GRAPHICS_FILES})
source_group("IO" FILES ${IO_FILES})
source_group("Importers" FILES ${IMPORTER_FILES})


target_link_libraries(Motor PRIVATE SDL3::SDL3)
target_link_libraries(Motor PRIVATE glad::glad)
target_link_libraries(Motor PRIVATE assimp::assimp)
target_link_libraries(Motor PRIVATE glm::glm)
target_link_libraries(Motor PRIVATE DevIL::IL)
target_link_libraries(Motor PRIVATE DevIL::ILU)
target_link_libraries(Motor PRIVATE imgui::imgui)
target_link_libraries(Motor PRIVATE imguizmo::imguizmo)
