cmake_minimum_required(VERSION 3.20)
project(twilight-elysium VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(te-engine STATIC
    src/engine/engine.cpp
    src/engine/memory.cpp
    src/engine/threading.h
    src/engine/input.cpp
    src/engine/resolution_scaler.cpp
    src/engine/math_types.cpp
    src/engine/entity.cpp
)
target_include_directories(te-engine PUBLIC src)

add_library(te-renderer STATIC
    src/renderer/renderer.cpp
    src/renderer/material/material.cpp
)
target_link_libraries(te-renderer PRIVATE te-engine)

add_library(te-shader STATIC
    src/shader/shader_stub.cpp
)
target_link_libraries(te-shader PRIVATE te-engine)

add_library(te-asset STATIC
    src/asset/asset_stub.cpp
)
target_link_libraries(te-asset PRIVATE te-engine)

add_library(te-pipeline STATIC
    src/pipeline/frame_graph_stub.cpp
)
target_link_libraries(te-pipeline PRIVATE te-engine)

add_library(te-lod STATIC
    src/game/legend_of_dragoon/character/character.cpp
    src/game/legend_of_dragoon/combat/combat.cpp
    src/game/legend_of_dragoon/world/world.cpp
    src/game/legend_of_dragoon/stage/stage.cpp
    src/game/legend_of_dragoon/legend_of_dragoon.cpp
    src/game/legend_of_dragoon/progression/progression.cpp
    src/game/legend_of_dragoon/menu/menu.cpp
    src/platform/local_runtime.cpp
    src/platform/satalight_runtime.cpp
    src/platform/runtime_factory.cpp
    src/platform/distribution/satalight_distributor.cpp
)
target_link_libraries(te-lod PRIVATE te-engine te-renderer te-shader te-pipeline)

enable_testing()
add_executable(te-test src/engine/main.cpp)
target_link_libraries(te-test PRIVATE te-engine te-lod)
# Headless bounded smoke: the engine loop must close and tick 60 frames
add_test(NAME te-core COMMAND te-test --bounded)
