# Spotlight Renderer

A DirectX 11 volumetric spotlight renderer implementing physical light behavior, shadow mapping, and Mie scattering.

## Features

- **Volumetric Lighting:** Real-time ray marching with Henyey-Greenstein phase function for realistic light scattering.
- **Physical Spotlight:** Inverse square law attenuation, adjustable beam and field angles (penumbra).
- **Gobo Projection:** Texture projection with rotation and shake effects.
- **Shadow Mapping:** High-resolution shadows casting through the volumetric fog.
- **CMY/RGBW Mixing:** Support for subtractive (CMY) and additive (RGB) color mixing models.
- **Performance:** Optimized ray marching with ray-sphere clipping and incremental light-space transforms.
- **Interactive UI:** Complete control over all parameters via ImGui.

## Requirements

- Windows 10/11
- Visual Studio 2019 or later (with C++ Desktop Development workload)
- CMake 3.15+
- DirectX 11 compatible GPU

## Build Instructions

1.  Clone the repository.
2.  Configure with CMake:
    ```powershell
    cmake -B build -S .
    ```
3.  Build the project:
    ```powershell
    cmake --build build --config Release
    ```
4.  Run the executable:
    ```powershell
    .\build\Release\SpotlightRenderer.exe
    ```

## Controls

The application launches with an ImGui overlay window "Spotlight Renderer Controls".

### Camera Controls
- **Distance:** Zoom in/out.
- **Pitch/Yaw:** Orbit around the target.
- **Target:** Move the look-at point.

### Spotlight Parameters
- **Position/Direction:** Move and aim the light.
- **Reset to Fixture:** Snaps the light back to the stage fixture model.
- **CMY Mixing:** Toggle between RGB and CMY color modes.
- **Beam/Field Angle:** Adjust the cone width and edge softness.

### Volumetric Quality
- **Step Count:** Increase for higher quality (more expensive), decrease for performance.
- **Density:** Thickness of the fog.
- **Anisotropy:** Controls how much light scatters forward vs backward (Mie scattering).

## Technical Details

- **Language:** C++17
- **Graphics API:** DirectX 11
- **Shader Model:** 5.0 (HLSL)
- **External Libraries:**
    - `ImGui`: User Interface
    - `tinyobjloader`: 3D Mesh Loading
    - `stb_image`: Texture Loading
