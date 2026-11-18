cmake_minimum_required(VERSION 3.16)
project(engine_watch VERSION 0.4 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(enginewatch_core STATIC src/core/EngineSimulation.cpp)
target_include_directories(enginewatch_core PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")

add_library(enginewatch_scene STATIC
    src/scene/World3D.cpp
    src/scene/Game3D.cpp
    src/scene/FontAtlas.cpp
)
target_include_directories(enginewatch_scene PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_link_libraries(enginewatch_scene PUBLIC enginewatch_core)

option(ENGINEWATCH_BUILD_GAME "Build interactive 3D game" ON)

if(CMAKE_SYSTEM_NAME STREQUAL "Generic" OR PSP OR PLATFORM_PSP)
    if(ENGINEWATCH_BUILD_GAME)
        add_executable(engine_watch
            src/psp/main.cpp
            src/psp/PspRenderer3D.cpp
        )
        target_include_directories(engine_watch PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
        target_link_libraries(engine_watch PRIVATE
            enginewatch_scene
            pspgum pspgu pspge pspvfpu pspdisplay pspctrl
            pspdebug pspuser
        )
        create_pbp_file(
            TARGET engine_watch
            ICON_PATH NULL
            BACKGROUND_PATH NULL
            PREVIEW_PATH NULL
            TITLE "ENGINE WATCH 3D v0.4"
            VERSION 01.00
        )
    endif()
elseif(ENGINEWATCH_BUILD_GAME)
    find_package(SDL2 QUIET CONFIG)
    if(NOT SDL2_FOUND)
        find_package(PkgConfig REQUIRED)
        pkg_check_modules(SDL2 REQUIRED IMPORTED_TARGET sdl2)
    endif()
    find_package(OpenGL REQUIRED)

    add_executable(engine_watch src/desktop/main.cpp)
    target_include_directories(engine_watch PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
    target_link_libraries(engine_watch PRIVATE enginewatch_scene OpenGL::GL)
    if(TARGET SDL2::SDL2-static)
        target_link_libraries(engine_watch PRIVATE SDL2::SDL2-static)
    elseif(TARGET SDL2::SDL2)
        target_link_libraries(engine_watch PRIVATE SDL2::SDL2)
    elseif(TARGET PkgConfig::SDL2)
        target_link_libraries(engine_watch PRIVATE PkgConfig::SDL2)
    endif()
endif()

include(CTest)
if(BUILD_TESTING AND NOT CMAKE_CROSSCOMPILING)
    add_executable(enginewatch_tests tests/EngineSimulationTests.cpp)
    target_link_libraries(enginewatch_tests PRIVATE enginewatch_core)
    add_test(NAME simulation_systems COMMAND enginewatch_tests)

    add_executable(enginewatch_world_tests tests/World3DTests.cpp)
    target_link_libraries(enginewatch_world_tests PRIVATE enginewatch_scene)
    add_test(NAME world_geometry_and_navigation COMMAND enginewatch_world_tests)
endif()
