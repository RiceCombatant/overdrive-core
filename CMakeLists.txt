cmake_minimum_required(VERSION 3.20)
project(OverdriveCore CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Prevent Windows min/max macros and winsock 1.x conflicts
add_compile_definitions(NOMINMAX WIN32_LEAN_AND_MEAN)

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

# miniaudio (Single-header audio playback and 3D spatial engine)
FetchContent_Declare(
    miniaudio
    GIT_REPOSITORY https://github.com/mackron/miniaudio.git
    GIT_TAG        0.11.21
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(miniaudio)

# OpenXR SDK (Khronos Group official loader for PCVR / Quest 3S)
set(BUILD_API_LAYERS OFF CACHE BOOL "" FORCE)
set(BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(BUILD_CONFORMANCE_TESTS OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
    OpenXR
    GIT_REPOSITORY https://github.com/KhronosGroup/OpenXR-SDK-Source.git
    GIT_TAG        release-1.1.43
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(OpenXR)

# fastgltf (High-performance C++20 glTF 2.0 / GLB parser)
set(FASTGLTF_DOWNLOAD_SIMDJSON ON CACHE INTERNAL "")
FetchContent_Declare(
    fastgltf
    GIT_REPOSITORY https://github.com/spnda/fastgltf.git
    GIT_TAG        v0.8.0
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(fastgltf)

# Source files
set(SOURCES
    src/main.cpp
    src/graphics/D3D11Renderer.cpp
    src/graphics/Camera.cpp
    src/graphics/GridFloor.cpp
    src/graphics/ModelLoader.cpp
    src/core/MechController.cpp
    src/physics/PhysicsManager.cpp
    src/combat/WeaponSystem.cpp
    src/combat/TargetDummy.cpp
    src/combat/TargetLockSystem.cpp
    src/combat/AIBotMech.cpp
    src/audio/AudioManager.cpp
    src/vr/VRManager.cpp
    src/network/NetworkManager.cpp
    src/ui/MenuSystem.cpp
    src/core/FrameSystem.cpp
    src/core/InternalSystem.cpp
    src/core/LoadoutManager.cpp
)

set(HEADERS
    src/graphics/D3D11Renderer.hpp
    src/graphics/Camera.hpp
    src/graphics/GridFloor.hpp
    src/graphics/ModelLoader.hpp
    src/core/MechController.hpp
    src/core/FrameSystem.hpp
    src/core/InternalSystem.hpp
    src/core/LoadoutManager.hpp
    src/physics/PhysicsManager.hpp
    src/combat/WeaponSystem.hpp
    src/combat/TargetDummy.hpp
    src/combat/TargetLockSystem.hpp
    src/combat/AIBotMech.hpp
    src/audio/AudioManager.hpp
    src/vr/VRManager.hpp
    src/network/NetworkManager.hpp
    src/ui/MenuSystem.hpp
    src/math/MathTypes.hpp
)

add_executable(OverdriveCore ${SOURCES} ${HEADERS})

target_compile_definitions(OverdriveCore PRIVATE
    NOMINMAX
    XR_USE_PLATFORM_WIN32
    XR_USE_GRAPHICS_API_D3D11
)

target_include_directories(OverdriveCore PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${jolt_SOURCE_DIR}
    ${miniaudio_SOURCE_DIR}
)

# Link Libraries
target_link_libraries(OverdriveCore PRIVATE
    SDL3::SDL3
    Jolt
    OpenXR::openxr_loader
    fastgltf::fastgltf
    d3d11
    dxgi
    d3dcompiler
    ws2_32
)

# Copy shaders and assets to output directory
add_custom_command(TARGET OverdriveCore POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/assets
        $<TARGET_FILE_DIR:OverdriveCore>/assets
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:SDL3::SDL3-shared>
        $<TARGET_FILE_DIR:OverdriveCore>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:OpenXR::openxr_loader>
        $<TARGET_FILE_DIR:OverdriveCore>
)
