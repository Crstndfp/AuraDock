cmake_minimum_required(VERSION 3.25)
project(cachy-dock VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# Optimization flags for CachyOS x86_64
add_compile_options(-Wall -Wextra -Wpedantic -O3)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Gui Quick Qml DBus)
find_package(KF6WindowSystem REQUIRED)
find_package(KF6Service REQUIRED)
find_package(LayerShellQt REQUIRED)
find_package(LibTaskManager REQUIRED)

# Domain, Application & Infrastructure library
add_library(cachy_dock_core
    src/domain/services/ParabolicMagnificationEngine.cpp
    src/domain/services/BouncePhysicsSolver.cpp
    src/domain/services/SmartAutoHideStateMachine.cpp
    src/application/DockController.cpp
    src/infrastructure/wayland/WaylandSurfaceMaskSynchronizer.cpp
    src/infrastructure/trackers/KWinWaylandTracker.cpp
    src/infrastructure/xdg/XdgDesktopRepository.cpp
)

target_include_directories(cachy_dock_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(cachy_dock_core PUBLIC
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
    Qt6::Qml
    Qt6::DBus
    KF6::WindowSystem
    KF6::Service
    LayerShellQt::Interface
    PW::LibTaskManager
)

# Main Executable
add_executable(cachy-dock
    src/main.cpp
)

target_compile_definitions(cachy-dock PRIVATE
    CACHY_DOCK_QML_DIR="${CMAKE_CURRENT_SOURCE_DIR}/src/ui/qml"
)

target_link_libraries(cachy-dock PRIVATE
    cachy_dock_core
)

# Unit Tests
option(BUILD_TESTING "Build unit tests" ON)
if(BUILD_TESTING)
    enable_testing()
    add_subdirectory(tests)
endif()
