# Spotlight Renderer

A technical demonstration of a physically-based volumetric lighting engine utilizing DirectX 11 and HLSL. This project showcases real-time ray-marching, shadow mapping, and hierarchical fixture loading via the General Device Type Format (GDTF) standard, built from scratch without commercial game engines.

[![Demo Video](https://img.youtube.com/vi/ap32ZcuJYaw/maxresdefault.jpg)](https://www.youtube.com/watch?v=ap32ZcuJYaw)
*[▶ Click for a preview of the renderer in action](https://www.youtube.com/watch?v=ap32ZcuJYaw)*

## Core Features

### 🔦 Volumetric Rendering
- **Real-Time Ray Marching:** GPU-based ray marching loop with dithering to simulate light interaction with atmospheric media.
- **Physical Scattering:** Implements the Henyey-Greenstein phase function for realistic Mie scattering (haze/fog).
- **Volumetric Shadows:** High-resolution shadow maps cast through the volumetric buffer, creating "god rays" and occlusion.
- **Post-Processing:** Kawase blur and FXAA for noise reduction and edge smoothing.

### 🏗️ GDTF & Scene Graph
- **GDTF Support:** Native parsing of `.gdtf` archives (ZIP), extracting `description.xml` and 3D assets. [GDTF](https://gdtf-share.com/) is an open standard for describing lighting fixtures.
- **Hierarchical Loading:** Reconstructs complex fixture geometry (Base → Yoke → Head) into a transformable scene graph.
- **Model Support:** Uses **Assimp** to load diverse mesh data (GLB, glTF, 3DS) embedded within fixtures.

### ⚙️ Engine Technology
- **Pure DirectX 11:** Custom C++17 renderer with no third-party engine dependencies.
- **Render Pipeline:** Multi-pass architecture:

```mermaid
flowchart LR
    subgraph Shadow
        A[Shadow Pass]
    end
    subgraph Lighting
        B[Scene Pass]
        C[Volumetric Pass]
    end
    subgraph Post-Processing
        D[Blur Pass]
        E[Composite Pass]
        F[FXAA Pass]
    end

    A -->|Shadow Map| B
    A -->|Shadow Map| C
    B -->|Scene RT| E
    C -->|Volume RT| D
    D -->|Blurred RT| E
    E -->|Combined RT| F
    F -->|Back Buffer| G((Display))
```
- **Interactive UI:** Powered by ImGui for real-time manipulation of light parameters (Beam Angle, Zoom, Color Mixing, Gobo Shake).

## Requirements

- **OS:** Windows 10/11
- **IDE:** Visual Studio 2022+ (C++ Desktop Development workload)
- **Build System:** CMake 3.15+
- **GPU:** DirectX 11 compatible hardware
- **Tools:** Git (for vcpkg), Ninja (included with Visual Studio)

## Build Instructions

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

3.  **Build** (uses Ninja, generates `compile_commands.json` for linting):
    ```powershell
    .\build.bat              # Debug build
    .\build.bat --release    # Release build
    ```

4.  **Run:**
    ```powershell
    .\run.bat              # Run Debug build
    .\run.bat --release    # Run Release build
    .\run.bat --build      # Build and run
    ```

5.  **Lint** (requires build first):
    ```powershell
    pwsh -File .\scripts\lint.ps1
    ```

## Controls

The application launches with an ImGui overlay window "Spotlight Renderer Controls".

### Camera
- **Distance / Pitch / Yaw:** Orbit around the scene.
- **Target:** Set the look-at point.

### Demo Effects
- **Enable Demo:** Toggle automated light animation.
- **Speed:** Animation playback rate.
- **Pan / Tilt / Rainbow / Gobo Rotation:** Toggle individual effects.

### Spotlight Parameters
- **Pan / Tilt:** GDTF fixture orientation.
- **Color / Intensity / Range:** Light appearance and reach.
- **Beam Angle / Field Angle:** Cone shape and softness.
- **Gobo:** Select pattern, rotation, and shake amount.

### Volumetric Quality
- **Step Count:** Ray-marching samples (quality vs. performance).
- **Density:** Atmospheric thickness.
- **Intensity Multiplier:** Volumetric brightness.
- **Anisotropy (g):** Forward vs. backward scattering.

### Post Processing
- **FXAA:** Anti-aliasing toggle.
- **Volumetric Blur:** Soften volumetric output.
- **Blur Passes:** Number of blur iterations.

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

## References

- Henyey, L. G., & Greenstein, J. L. (1941). Diffuse radiation in the galaxy. *Astrophysical Journal*, 93, 70-83. [[ADS](https://ui.adsabs.harvard.edu/abs/1941ApJ....93...70H)]
- Williams, L. (1978). Casting curved shadows on curved surfaces. *Proceedings of the 5th Annual Conference on Computer Graphics and Interactive Techniques (SIGGRAPH)*, 270-274. [[ACM](https://doi.org/10.1145/800248.807402)]
- Blinn, J. F. (1977). Models of light reflection for computer synthesized pictures. *Proceedings of the 4th Annual Conference on Computer Graphics and Interactive Techniques (SIGGRAPH)*, 192-198. [[ACM](https://doi.org/10.1145/563858.563893)]
- Lottes, T. (2011). *FXAA 3.11*. NVIDIA. [[PDF](https://developer.download.nvidia.com/assets/gamedev/files/sdk/11/FXAA_WhitePaper.pdf)]
- Jimenez, J. (2014). Next generation post processing in Call of Duty: Advanced Warfare. *SIGGRAPH Advances in Real-Time Rendering in Games*. [[Slides](https://www.iryoku.com/next-generation-post-processing-in-call-of-duty-advanced-warfare/)]
