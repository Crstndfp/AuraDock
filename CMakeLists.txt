cmake_minimum_required(VERSION 3.25)
project(cachy-dock VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Optimization flags for CachyOS x86_64
add_compile_options(-Wall -Wextra -Wpedantic -O3)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Gui Quick Qml)
find_package(KF6WindowSystem REQUIRED)
find_package(KF6Service REQUIRED)
find_package(LayerShellQt REQUIRED)

# Core domain & application library
add_library(cachy_dock_core
    src/domain/services/ParabolicMagnificationEngine.cpp
    src/domain/services/BouncePhysicsSolver.cpp
    src/domain/services/SmartAutoHideStateMachine.cpp
)

target_include_directories(cachy_dock_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(cachy_dock_core PUBLIC
    Qt6::Core
)

# Unit Tests
option(BUILD_TESTING "Build unit tests" ON)
if(BUILD_TESTING)
    enable_testing()
    add_subdirectory(tests)
endif()
