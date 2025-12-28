# Spotlight Renderer

A technical demonstration of a physically-based volumetric lighting engine utilizing DirectX 11 and HLSL. This project showcases real-time ray-marching, shadow mapping, and hierarchical fixture loading via the General Device Type Format (GDTF) standard, built from scratch without commercial game engines.

## Core Features

### 🔦 Volumetric Rendering
- **Real-Time Ray Marching:** GPU-based ray marching loop with dithering to simulate light interaction with atmospheric media.
- **Physical Scattering:** Implements the Henyey-Greenstein phase function for realistic Mie scattering (haze/fog).
- **Volumetric Shadows:** High-resolution shadow maps cast through the volumetric buffer, creating "god rays" and occlusion.
- **Post-Processing:** Kawase blur and FXAA for noise reduction and edge smoothing.

### 🏗️ GDTF & Scene Graph
- **GDTF Support:** Native parsing of `.gdtf` archives (ZIP), extracting `description.xml` and 3D assets.
- **Hierarchical Loading:** Reconstructs complex fixture geometry (Base → Yoke → Head) into a transformable scene graph.
- **Model Support:** Uses **Assimp** to load diverse mesh data (GLB, glTF, 3DS) embedded within fixtures.

### ⚙️ Engine Technology
- **Pure DirectX 11:** Custom C++17 renderer with no third-party engine dependencies.
- **Render Pipeline:** Multi-pass architecture:
  1.  **Shadow Pass:** Depth rendering from light perspective.
  2.  **Scene Pass:** Opaque geometry rendering.
  3.  **Volumetric Pass:** Ray-marching accumulation into off-screen buffer.
  4.  **Blur Pass:** Multi-tap Gaussian/Kawase blur on volumetric data.
  5.  **Composite Pass:** Additive blending of scene and fog.
  6.  **FXAA Pass:** Anti-aliasing.
- **Interactive UI:** Powered by ImGui for real-time manipulation of light parameters (Beam Angle, Zoom, Color Mixing, Gobo Shake).

## Requirements

- **OS:** Windows 10/11
- **IDE:** Visual Studio 2019 or later (C++ Desktop Development workload)
- **Build System:** CMake 3.15+
- **GPU:** DirectX 11 compatible hardware
- **Tools:** Git (for vcpkg)
- **Optional:** Ninja (recommended for faster builds and linting support)

## Build Instructions

### Standard (Visual Studio)

1.  **Clone the repository:**
    ```powershell
    git clone https://github.com/your-repo/spotlight-renderer.git
    cd spotlight-renderer
    ```

2.  **Setup vcpkg** (first time only):
    ```powershell
    git clone https://github.com/Microsoft/vcpkg.git
    .\vcpkg\bootstrap-vcpkg.bat
    ```

3.  **Configure with CMake:**
    ```powershell
    cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE="vcpkg/scripts/buildsystems/vcpkg.cmake"
    ```

4.  **Build the project:**
    ```powershell
    cmake --build build --config Release
    ```

5.  **Run the executable:**
    ```powershell
    .\build\Release\SpotlightRenderer.exe
    ```

### Advanced (Ninja / CI Workflow)

To reproduce the CI environment or run the linter (which requires `compile_commands.json`), usage of **Ninja** is recommended.

1.  **Configure:**
    ```powershell
    cmake -B build -S . -G Ninja -DCMAKE_TOOLCHAIN_FILE="vcpkg/scripts/buildsystems/vcpkg.cmake" -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
    ```

2.  **Build:**
    ```powershell
    cmake --build build --config Release
    ```

3.  **Run:**
    *Note: Ninja places binaries directly in `build/`, not `build/Release/`.*
    ```powershell
    .\build\SpotlightRenderer.exe
    ```

4.  **Linting:**
    ```powershell
    .\scripts\lint.ps1
    ```

## Controls

The application launches with an ImGui overlay window "Spotlight Renderer Controls".

### Camera
- **Right Click + Drag:** Orbit camera.
- **Middle Click + Drag:** Pan camera.
- **Scroll:** Zoom in/out.

### Spotlight Parameters
- **Position/Direction:** Move and aim the light in 3D space.
- **GDTF Node Control:** Rotate specific parts of the fixture (e.g., Pan/Tilt).
- **Beam Properties:** Adjust Beam Angle (cone) and Field Angle (softness).
- **Color:** Toggle between RGB and CMY subtractive mixing.
- **Gobo:** Enable rotation and shake effects.

### Volumetric Quality
- **Step Count:** Number of ray-marching samples (higher = better quality, lower performance).
- **Density:** Atmospheric thickness.
- **Anisotropy (g):** Controls forward vs. backward light scattering.

## Technical Details

- **Language:** C++17
- **Graphics API:** DirectX 11
- **Shader Model:** 5.0 (HLSL)
- **External Libraries:**
    - `ImGui`: User Interface
    - `tinyobjloader`: OBJ Mesh Loading (Static geometry)
    - `assimp`: GLB, glTF, 3DS Mesh Loading (Fixture geometry)
    - `stb_image`: Texture Loading
    - `pugixml`: XML Parsing (GDTF)
    - `miniz`: Archive extraction (GDTF)
