cmake_minimum_required(VERSION 3.16)
project(engine_watch VERSION 0.2 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(enginewatch_core STATIC src/core/EngineSimulation.cpp)
target_include_directories(enginewatch_core PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")

option(ENGINEWATCH_BUILD_GAME "Build SDL2 application (requires SDL2)" ON)

if(ENGINEWATCH_BUILD_GAME)
    find_package(SDL2 QUIET CONFIG)
    if(NOT SDL2_FOUND)
        find_package(PkgConfig QUIET)
        if(PkgConfig_FOUND)
            pkg_check_modules(SDL2 REQUIRED IMPORTED_TARGET sdl2)
        else()
            message(FATAL_ERROR "SDL2 not found. Install SDL2 or use -DENGINEWATCH_BUILD_GAME=OFF for tests.")
        endif()
    endif()

    add_executable(engine_watch
        src/main.cpp
        src/ui/BitmapFont.cpp
        src/ui/GameApp.cpp
    )
    target_link_libraries(engine_watch PRIVATE enginewatch_core)
    target_include_directories(engine_watch PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")

    if(TARGET SDL2::SDL2-static)
        target_link_libraries(engine_watch PRIVATE SDL2::SDL2-static)
    elseif(TARGET SDL2::SDL2)
        target_link_libraries(engine_watch PRIVATE SDL2::SDL2)
    elseif(TARGET PkgConfig::SDL2)
        target_link_libraries(engine_watch PRIVATE PkgConfig::SDL2)
    else()
        target_include_directories(engine_watch PRIVATE ${SDL2_INCLUDE_DIRS})
        target_link_libraries(engine_watch PRIVATE ${SDL2_LIBRARIES})
    endif()

    if(CMAKE_SYSTEM_NAME STREQUAL "Generic" OR PSP)
        target_link_libraries(engine_watch PRIVATE
            pspdebug pspvram pspvfpu pspdisplay pspgu pspge pspaudio pspctrl psppower pspuser
        )
        create_pbp_file(
            TARGET engine_watch
            ICON_PATH NULL
            BACKGROUND_PATH NULL
            PREVIEW_PATH NULL
            TITLE "ENGINE WATCH v0.2"
            VERSION 01.00
        )
    endif()
endif()

include(CTest)
if(BUILD_TESTING AND NOT CMAKE_CROSSCOMPILING)
    add_executable(enginewatch_tests tests/EngineSimulationTests.cpp)
    target_link_libraries(enginewatch_tests PRIVATE enginewatch_core)
    add_test(NAME simulation_systems COMMAND enginewatch_tests)
endif()
