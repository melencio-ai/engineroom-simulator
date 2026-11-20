cmake_minimum_required(VERSION 3.16)
project(engine_watch VERSION 0.7.1 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(enginewatch_core STATIC src/core/EngineSimulation.cpp)
target_include_directories(enginewatch_core PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")

add_library(enginewatch_scene STATIC
    src/scene/DeckLayout.cpp
    src/scene/MarineParts.cpp
    src/scene/World3D.cpp
    src/scene/Game3D.cpp
    src/scene/PlayerCharacter.cpp
    src/scene/ImportedHumanoid.cpp
    src/scene/ImportedHumanoid.cpp
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
            TITLE "ENGINE WATCH G3.7.1 HUMAN"
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

# Export individual portable 3D mesh files (.obj + .mtl) from the same
# 33 reusable prototypes used by the PSP game. Native-host tool only.
option(ENGINEWATCH_EXPORT_ASSETS "Build OBJ exporter for the marine parts library" OFF)
if(ENGINEWATCH_EXPORT_ASSETS AND NOT CMAKE_CROSSCOMPILING)
    add_executable(enginewatch_export_parts tools/export_marine_parts.cpp)
    target_link_libraries(enginewatch_export_parts PRIVATE enginewatch_scene)
endif()

# G3.5: native OBJ snapshots of the same procedural character that
# animates in PSP and desktop. Native-only, never bundled into PSP EBOOT.
option(ENGINEWATCH_EXPORT_CHARACTER "Export low-poly marine-engineer character poses" OFF)
if(ENGINEWATCH_EXPORT_CHARACTER AND NOT CMAKE_CROSSCOMPILING)
    add_executable(enginewatch_export_character tools/export_player_character.cpp)
    target_link_libraries(enginewatch_export_character PRIVATE enginewatch_scene)
endif()

# G3: native constructor exporter reads the very same DeckPlan as the
# playable C++ engine to generate physical layout JSON and vector deck maps.
option(ENGINEWATCH_EXPORT_DECKPLAN "Export shared deck layout JSON and SVG blueprints" OFF)
if(ENGINEWATCH_EXPORT_DECKPLAN AND NOT CMAKE_CROSSCOMPILING)
    add_executable(enginewatch_export_deckplan tools/export_deck_plan.cpp)
    target_link_libraries(enginewatch_export_deckplan PRIVATE enginewatch_scene)
endif()

include(CTest)
if(BUILD_TESTING AND NOT CMAKE_CROSSCOMPILING)
    add_executable(enginewatch_tests tests/EngineSimulationTests.cpp)
    target_link_libraries(enginewatch_tests PRIVATE enginewatch_core)
    add_test(NAME simulation_systems COMMAND enginewatch_tests)

    add_executable(enginewatch_world_tests tests/World3DTests.cpp)
    target_link_libraries(enginewatch_world_tests PRIVATE enginewatch_scene)
    add_test(NAME world_geometry_and_navigation COMMAND enginewatch_world_tests)

    add_executable(enginewatch_deck_tests tests/DeckLayoutTests.cpp)
    target_link_libraries(enginewatch_deck_tests PRIVATE enginewatch_scene)
    add_test(NAME constructed_deck_geometry_and_collision COMMAND enginewatch_deck_tests)

    add_executable(enginewatch_player_tests tests/PlayerCharacterTests.cpp)
    target_link_libraries(enginewatch_player_tests PRIVATE enginewatch_scene)
    add_test(NAME marine_engineer_character_and_camera COMMAND enginewatch_player_tests)

    add_executable(enginewatch_interaction_tests tests/InteractionSocketTests.cpp)
    target_link_libraries(enginewatch_interaction_tests PRIVATE enginewatch_scene)
    add_test(NAME physical_equipment_sockets_and_hand_contact COMMAND enginewatch_interaction_tests)

    add_executable(enginewatch_humanoid_tests tests/ImportedHumanoidTests.cpp)
    target_link_libraries(enginewatch_humanoid_tests PRIVATE enginewatch_scene)
    add_test(NAME imported_quaternius_humanoid_clips COMMAND enginewatch_humanoid_tests)

    add_executable(enginewatch_parts_tests tests/MarinePartsTests.cpp)
    target_link_libraries(enginewatch_parts_tests PRIVATE enginewatch_scene)
    add_test(NAME reusable_marine_component_catalog COMMAND enginewatch_parts_tests)
endif()
