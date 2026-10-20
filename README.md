# cachy-dock: macOS-Style Glassmorphic Dock for CachyOS & Wayland

A modern, high-performance macOS-style dock built with C++23, Qt 6.12 Quick/QML, KDE Frameworks 6, and LayerShellQt for CachyOS and Arch Linux running Wayland (KWin / wlroots).

## Features
- **Continuous Parabolic & Wave Magnification**: Smooth raised-cosine wave scaling with horizontal expansion outward from the cursor anchor.
- **Glassmorphic Dual-Border Design**: Dual-kawase hardware background blur via `org_kde_kwin_blur_manager` (`KWindowEffects`) and dynamic `QRegion` mask synchronization.
- **macOS Status Indicators**: Glowing silver dots for running background apps, smoothly morphing into wide luminous accent capsules for active focused windows.
- **Damped Harmonic Bounce Physics**: Damped oscillator simulation with impact squash-and-stretch on app launch.
- **Smart Auto-Hide**: Detects maximized or fullscreen windows on the active monitor, collapsing to a 2px bottom edge sensor with dwell detection to free screen space.
- **Recent Apps & Document Stacks**: Categorized sections for pinned apps, recently launched apps, and document folder stacks (`~/Downloads`) with fan/grid popovers.
- **Glassmorphic Context Menu**: Right-click menu for new instances, keeping/removing from dock, launching with discrete GPU (`DRI_PRIME=1`), and quitting apps.

## Requirements
- CachyOS / Arch Linux (Wayland session, e.g. KDE Plasma 6 or Hyprland)
- Qt 6.5+ (`qt6-base`, `qt6-declarative`)
- KDE Frameworks 6 (`kwindowsystem`, `kservice`)
- `layer-shell-qt`
- CMake 3.25+ and C++23 compiler (`g++` or `clang++`)

## Build & Test
```bash
# Configure and build
cmake -B build -G "Unix Makefiles" -DCMAKE_CXX_COMPILER=g++
cmake --build build -j$(nproc)

# Run unit tests (100% pass rate)
ctest --test-dir build --output-on-failure
```

## Running
```bash
./build/cachy-dock
```
