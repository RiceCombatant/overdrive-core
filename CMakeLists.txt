cmake_minimum_required(VERSION 3.20)
project(OverdriveCore CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Prevent Windows min/max macros from conflicting with std/Jolt
add_compile_definitions(NOMINMAX)

# SDL3
include(FetchContent)
FetchContent_Declare(
    SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG        release-3.2.8
    GIT_SHALLOW    TRUE
)
set(SDL_SHARED ON CACHE BOOL "" FORCE)
set(SDL_STATIC OFF CACHE BOOL "" FORCE)
set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(SDL3)

# Jolt Physics
FetchContent_Declare(
    Jolt
    GIT_REPOSITORY https://github.com/jrouwe/JoltPhysics.git
    GIT_TAG        v5.6.0
    GIT_SHALLOW    TRUE
    SOURCE_SUBDIR  Build
)
set(TARGET_UNIT_TESTS OFF CACHE BOOL "" FORCE)
set(TARGET_HELLO_WORLD OFF CACHE BOOL "" FORCE)
set(TARGET_PERFORMANCE_TEST OFF CACHE BOOL "" FORCE)
set(TARGET_SAMPLES OFF CACHE BOOL "" FORCE)
set(TARGET_VIEWER OFF CACHE BOOL "" FORCE)
set(USE_STATIC_MSVC_RUNTIME_LIBRARY OFF CACHE BOOL "" FORCE)
set(OVERRIDE_CXX_FLAGS OFF CACHE BOOL "" FORCE)
set(INTERPROCEDURAL_OPTIMIZATION OFF CACHE BOOL "" FORCE)
set(FLOATING_POINT_EXCEPTIONS_ENABLED OFF CACHE BOOL "" FORCE)
set(CROSS_COMPILE_ARM OFF CACHE BOOL "" FORCE)
set(DOUBLE_PRECISION OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(Jolt)

# Source files
set(SOURCES
    src/main.cpp
    src/graphics/D3D11Renderer.cpp
    src/graphics/Camera.cpp
    src/graphics/GridFloor.cpp
    src/core/MechController.cpp
    src/physics/PhysicsManager.cpp
    src/combat/WeaponSystem.cpp
    src/combat/TargetDummy.cpp
    src/combat/TargetLockSystem.cpp
)

set(HEADERS
    src/graphics/D3D11Renderer.hpp
    src/graphics/Camera.hpp
    src/graphics/GridFloor.hpp
    src/core/MechController.hpp
    src/physics/PhysicsManager.hpp
    src/combat/WeaponSystem.hpp
    src/combat/TargetDummy.hpp
    src/combat/TargetLockSystem.hpp
    src/math/MathTypes.hpp
)

add_executable(OverdriveCore ${SOURCES} ${HEADERS})

target_include_directories(OverdriveCore PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${jolt_SOURCE_DIR}
)

# Link Libraries
target_link_libraries(OverdriveCore PRIVATE
    SDL3::SDL3
    Jolt
    d3d11
    dxgi
    d3dcompiler
)

# Copy shaders and assets to output directory
add_custom_command(TARGET OverdriveCore POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/assets
        $<TARGET_FILE_DIR:OverdriveCore>/assets
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:SDL3::SDL3-shared>
        $<TARGET_FILE_DIR:OverdriveCore>
)
