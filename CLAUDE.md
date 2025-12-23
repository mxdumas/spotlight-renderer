# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Spotlight Renderer is a DirectX 11 volumetric lighting simulator implementing physically-based stage lighting with real-time ray-marching, shadow mapping, and Mie scattering effects. Supports GDTF fixture loading with hierarchical scene graphs.

## Build Commands

```powershell
# Setup vcpkg (first time only)
git clone https://github.com/Microsoft/vcpkg.git
.\vcpkg\bootstrap-vcpkg.bat

# Configure (generates compile_commands.json for clang-tidy)
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE="vcpkg/scripts/buildsystems/vcpkg.cmake"

# Build Release
cmake --build build --config Release

# Build Debug
cmake --build build --config Debug

# Run
.\build\Release\SpotlightRenderer.exe

# Run all tests
ctest --test-dir ./build -C Release

# Run single test
.\build\Release\TestRayMath.exe
.\build\Release\TestVolumetricJitter.exe

# Lint (requires VS 2022+ with clang tools)
.\scripts\lint.ps1
```

## Architecture

The application follows a layered architecture with `Application` as the coordinator:

```
Application
├── GraphicsDevice     # D3D11 device, context, swap chain
├── Scene              # Camera, spotlights, meshes, textures
├── RenderPipeline     # Orchestrates render passes
├── UIRenderer         # ImGui integration
└── GeometryGenerator  # Procedural room geometry
```

### Render Pipeline Flow

Each frame executes these passes in sequence:
1. **ShadowPass** → generates shadow map from spotlight's perspective
2. **ScenePass** → renders geometry with lighting to sceneRT
3. **VolumetricPass** → ray marching for volumetric effects to volRT
4. **BlurPass** → Kawase gaussian blur on volumetric buffer
5. **CompositePass** → combines sceneRT + blurred volRT
6. **FXAAPass** → anti-aliasing to backbuffer

### Key Directories

- `src/Core/` - GraphicsDevice, ConstantBuffer template, Config.h (all magic numbers)
- `src/Scene/` - Scene container, Camera, Spotlight, CeilingLights, SceneGraph (Node hierarchy)
- `src/Rendering/Passes/` - Individual render pass implementations (IRenderPass interface)
- `src/Resources/` - Mesh (OBJ+MTL loader), Shader (HLSL compilation), Texture, GDTFLoader/Parser
- `shaders/` - HLSL Shader Model 5.0 files

### GDTF Loading

GDTF fixtures are loaded via a two-stage process:
1. **GDTFParser** - Extracts and parses the `.gdtf` archive (ZIP containing `description.xml` + 3D models)
2. **GDTFLoader** - Converts parsed geometry into a `SceneGraph::Node` hierarchy

The scene graph uses `Node` as a base class with `MeshNode` for renderable geometry. Transforms propagate parent→child via `updateWorldMatrix()`.

### Configuration

All constants are centralized in `src/Core/Config.h` with namespaces: `Config::Display`, `Config::Shadow`, `Config::Volumetric`, `Config::Spotlight`, etc.

## Code Style

### C++17
- Classes/Structs: PascalCase (`class NetworkScanner`)
- Functions/Methods: camelCase (`void sendMessage()`)
- Local variables: snake_case (`auto retry_count = 0`)
- Private members: snake_case with trailing underscore (`int connection_id_`)
- Constants/Enums: SCREAMING_SNAKE (`static constexpr int MAX_BUFFER`)
- Use `ComPtr<T>` for all DirectX COM objects, never raw pointers
- Constant buffer structs must be 16-byte aligned: `static_assert(sizeof(T) % 16 == 0)`

### HLSL
- IO Structures: PascalCase with suffix (`struct VS_INPUT`, `struct PS_OUTPUT`)
- Functions: PascalCase (`float3 CalculateLighting()`)
- Local variables: snake_case (`float3 light_dir`)
- Textures/Samplers: prefix `t_`/`s_` (`Texture2D t_diffuse`, `SamplerState s_linear`)
- Always declare registers explicitly (`register(b0)`, `register(t0)`)
- Use coordinate space suffixes: `_world`, `_view`, `_clip`, `_model`

## External Dependencies

- **ImGui** (`external/imgui/`) - UI
- **tinyobjloader** (`external/tiny_obj_loader.h`) - OBJ parsing
- **stb_image** (`external/stb_image.h`) - Texture loading
- **pugixml** (`external/pugixml/`) - XML parsing for GDTF description.xml
- **miniz** (via vcpkg) - ZIP extraction for .gdtf archives
- **tinygltf** (`external/tinygltf/`) - glTF/GLB model loading
