<div align="center">

# ✨ AuraDock

**A Modern, High-Performance Glassmorphic Dock for Wayland Desktops**

[![C++23](https://img.shields.io/badge/C%2B%2B-23-00599C?logo=cplusplus)](https://en.wikipedia.org/wiki/C%2B%2B23)
[![Qt 6](https://img.shields.io/badge/Qt-6.5+-41CD52?logo=qt)](https://www.qt.io/)
[![KDE Frameworks 6](https://img.shields.io/badge/KDE-KF6-1D99F3?logo=kde)](https://kde.org/)
[![Wayland LayerShell](https://img.shields.io/badge/Wayland-LayerShell-FFA500?logo=wayland)](https://wayland.freedesktop.org/)
[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)
[![Unit Tests](https://img.shields.io/badge/Tests-56%2F56%20Passed-brightgreen.svg)](#testing)

*AuraDock brings the fluid physics, refined aesthetic, and seamless window management of macOS to Linux Wayland compositors (KWin, wlroots).*

</div>

---

## 🌟 Key Features

### 🌊 Continuous Parabolic & Wave Magnification
- **Raised-Cosine Bell Curve**: Smooth, continuous $C^1$ derivative scaling with outward lateral displacement for neighboring icons.
- **Unified Vertical Baseline**: Icons expand upwards maintaining a stable baseline and natural cursor centering.
- **Approach Velocity Sensing**: Gradual smoothstep magnification as the cursor approaches vertically from above.

### 🧞 Dual-Layer Genie Lamp Effect (*Magic Lamp*)
- **In-Dock Spirit Emergence**: Clicking a minimized app or instance card triggers an S-curve Bézier genie funnel with horizontal scanline distortion and icon spout deformation.
- **Compositor Integration**: Hooks directly into KWin's native `magiclamp` effect via Wayland protocol (`org_kde_plasma_window_set_minimized_geometry`), unminimizing windows directly from the exact coordinate of the dock icon.

### 🧠 Geometry-Aware Smart Auto-Hide (*Window Dodging*)
- **Multi-Scenario Intelligence**: Automatically hides when any visible window spans from top to dock (including half-tiled `Super+Left`/`Right` or narrow full-height windows), or when resized tall windows approach the dock.
- **Monotonic Stability**: Evaluates dimensions against stable configuration baselines to eliminate oscillation loops.
- **Transient Sensor Strip**: Collapses to an unobtrusive 4px bottom sensor with dwell detection to reveal the dock instantly.
- **Active Menu Protection**: Dock remains anchored while interacting with preferences, context menus, or multi-instance popups.

### 💎 Crystal Glassmorphism & Full Transparency
- **Hardware Dual-Kawase Blur**: Leverages `org_kde_kwin_blur_manager` via `KWindowEffects` for real-time backdrop blur without compositor stutter.
- **Dynamic Opacity Engine**: Tunable from opaque frosted glass to 100% crystal transparency without amber/golden tint distortions.
- **Zero-Ghost Click Pass-Through**: Bounded Wayland LayerShell input mask strictly encapsulates the interactive pill and magnified icons; background application windows directly behind unoccupied areas receive 100% of mouse clicks.

### 🗂️ Task & Window Management
- **Multi-Instance Previews**: Grouped running apps offer an interactive popover preview card switcher or list mode.
- **Drag & Drop Reordering**: Reorganize pinned application launchers smoothly in real time.
- **Document & Folder Stacks**: Quick-access popup fan and grid views for folders (such as `~/Downloads`).
- **Glassmorphic Context Menu**: Pin/unpin launchers, launch on discrete GPU (`DRI_PRIME=1`), close windows, or insert separators.
- **Preferences Panel**: Real-time GUI adjustments for bar height, bottom margin, icon size, spacing, corner radius, opacity, and blur.

---

## 🏗️ System Architecture

AuraDock is structured using clean architecture and SOLID principles:

```
src/
├── domain/                      # Pure business logic & physics engines
│   ├── entities/                # WindowInstance, DockItem, DockSettings
│   ├── interfaces/              # IWindowTracker abstractions
│   └── services/
│       ├── ParabolicMagnificationEngine.cpp   # Raised-cosine magnification math
│       ├── BouncePhysicsSolver.cpp            # Damped oscillator & squash-stretch
│       └── SmartAutoHideStateMachine.cpp      # Monotonic dodge & auto-hide states
├── application/
│   └── DockController.cpp       # QAbstractListModel bridge between domain and QML
├── infrastructure/              # Desktop environment & Wayland integrations
│   ├── trackers/                # KWinWaylandTracker (KDE LibTaskManager & D-Bus)
│   ├── wayland/                 # WaylandSurfaceMaskSynchronizer (LayerShell & QRegion)
│   └── xdg/                     # XDG Desktop files & ThemeIconProvider
└── ui/
    └── qml/                     # Declarative UI (MainDock, GlassPill, Popups, Genie)
```

---

## 📦 Requirements & Dependencies

AuraDock is engineered for Linux distributions running Wayland (e.g. CachyOS, Arch Linux, Fedora, openSUSE):

| Component | Required Package / Library |
| :--- | :--- |
| **Compiler** | GCC 13+ or Clang 16+ (with full C++23 support) |
| **Build System** | CMake 3.25+ |
| **Qt 6 Framework** | `qt6-base`, `qt6-declarative`, `qt6-wayland` |
| **KDE Frameworks 6** | `kwindowsystem`, `kservice`, `plasma-workspace` (`libtaskmanager`) |
| **Wayland Protocols** | `layer-shell-qt`, `wayland-protocols` |
| **Testing** | GoogleTest (`gtest`) |

### Installing Dependencies on Arch / CachyOS:
```bash
sudo pacman -S --needed \
    base-devel cmake git \
    qt6-base qt6-declarative qt6-wayland \
    layer-shell-qt kwindowsystem kservice plasma-workspace \
    gtest
```

---

## 🚀 Building & Running

### 1. Clone the Repository
```bash
git clone https://github.com/Crstndfp/AuraDock.git
cd AuraDock
```

### 2. Configure & Build
```bash
cmake -B build -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
```

### 3. Run Unit Tests
```bash
./build/tests/cachy_dock_tests
# Or with CTest:
ctest --test-dir build --output-on-failure
```

### 4. Launch AuraDock
```bash
./build/cachy-dock
```

---

## ⚙️ Configuration

Configuration is automatically synchronized and stored using standard XDG specifications at:
```
~/.config/cachy-dock/dock.conf
```

Key tunable settings include:
- `appearance/barHeight`: Base height of the frosted pill.
- `appearance/bottomMargin`: Screen margin from the bottom output edge.
- `appearance/iconSize`: Base icon resolution.
- `appearance/itemSpacing`: Gap between adjacent icons.
- `appearance/dockOpacity`: Glassmorphic opacity ($0.0$ to $1.0$).
- `appearance/dockTransparency`: Inverse opacity value ($1.0 - \text{dockOpacity}$).
- `appearance/blurEnabled`: Toggles real-time compositor backdrop blur.
- `appearance/multiInstanceMode`: `0` for window mini cards, `1` for compact list.

---

## 🧪 Testing

AuraDock comes with an automated GoogleTest suite covering 100% of core domain services:
- **`ParabolicMagnificationEngineTest`**: Continuous boundary differentiability, cosine factor curves, and displacement bounds.
- **`BouncePhysicsSolverTest`**: Harmonic oscillation, decay envelopes, and squash/stretch kinematics.
- **`SmartAutoHideStateMachineTest`**: Window geometry evaluation, edge sensor transitions, and monotonic state stability.
- **`DockWindowTrackingTest`**: Multi-window aggregation, reverse-DNS desktop resolution, and Genie effect triggers.
- **`DockSettingsTest`**: Dynamic settings persistence, corner radius adaptation, and mask emission integrity.

Run the test suite with:
```bash
ctest --test-dir build --verbose
```

---

## 📄 License

This project is licensed under the **GNU General Public License v3.0 (GPL-3.0)**.

> **Open-Source Guarantee**: Under the GPL-3.0, any derivative works, modifications, or software incorporating AuraDock code must also be released as open-source under the same terms and made publicly available. Use in closed-source or proprietary software is prohibited.

See the [LICENSE](LICENSE) file for complete details.

---

<div align="center">
  <sub>Developed by <b>Cristian Flores</b> (<a href="mailto:thecris.danny@gmail.com">thecris.danny@gmail.com</a>)</sub>
</div>
