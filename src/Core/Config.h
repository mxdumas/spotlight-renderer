#pragma once

#include <cstdint>

namespace Config {

    // ===========================================
    // Math Constants
    // ===========================================
    namespace Math {
        constexpr float PI = 3.14159265358979323846f;
        constexpr float PI_DIV_2 = PI / 2.0f;
        constexpr float PI_DIV_4 = PI / 4.0f;
    }

    // ===========================================
    // Display Configuration
    // ===========================================
    namespace Display {
        constexpr int WINDOW_WIDTH = 1920;
        constexpr int WINDOW_HEIGHT = 1080;
        constexpr float ASPECT_RATIO = static_cast<float>(WINDOW_WIDTH) / static_cast<float>(WINDOW_HEIGHT);
        constexpr int REFRESH_RATE = 60;
    }

    // ===========================================
    // Shadow Mapping
    // ===========================================
    namespace Shadow {
        constexpr int MAP_SIZE = 2048;
    }

    // ===========================================
    // Room Geometry
    // ===========================================
    namespace Room {
        constexpr float HALF_WIDTH = 50.0f;
        constexpr float FLOOR_Y = -0.05f;
        constexpr float CEILING_Y = 100.0f;
        constexpr int INDEX_COUNT = 36; // 6 faces * 2 triangles * 3 vertices
    }

    // ===========================================
    // Camera Defaults
    // ===========================================
    namespace CameraDefaults {
        constexpr float DISTANCE = 40.0f;
        constexpr float PITCH = 0.4f;
        constexpr float YAW = 0.0f;
        constexpr float FOV = Math::PI_DIV_4; // 45 degrees
        constexpr float CLIP_NEAR = 0.1f;
        constexpr float CLIP_FAR = 1000.0f;
    }

    // ===========================================
    // Spotlight Defaults
    // ===========================================
    namespace Spotlight {
        constexpr float DEFAULT_RANGE = 500.0f;
        constexpr float DEFAULT_INTENSITY = 100.0f;
        constexpr float DEFAULT_BEAM_ANGLE = 0.98f;
        constexpr float DEFAULT_FIELD_ANGLE = 0.71f;
        constexpr float DEFAULT_HEIGHT = 15.0f;

        // Gobo shake effect
        constexpr float SHAKE_SCALE = 0.05f;
        constexpr float SHAKE_FREQ_X = 30.0f;
        constexpr float SHAKE_FREQ_Y = 35.0f;
    }

    // ===========================================
    // Volumetric Lighting Defaults
    // ===========================================
    namespace Volumetric {
        constexpr float DEFAULT_STEP_COUNT = 512.0f;
        constexpr float MIN_STEP_COUNT = 16.0f;
        constexpr float MAX_STEP_COUNT = 512.0f;
        constexpr float DEFAULT_DENSITY = 0.2f;
        constexpr float DEFAULT_INTENSITY = 10.0f;
        constexpr float DEFAULT_ANISOTROPY = 0.5f;
        constexpr float MIN_ANISOTROPY = -0.99f;
        constexpr float MAX_ANISOTROPY = 0.99f;
        constexpr float JITTER_SCALE = 0.005f;
    }

    // ===========================================
    // Ceiling Lights Configuration
    // ===========================================
    namespace CeilingLights {
        constexpr float HEIGHT = 95.0f;
        constexpr float RANGE = 200.0f;
        constexpr float X_START = -40.0f;
        constexpr float Z_START = -20.0f;
        constexpr float X_SPACING = 26.6f;
        constexpr float Z_SPACING = 40.0f;
        constexpr int GRID_X = 4;
        constexpr int GRID_Z = 2;
        constexpr int TOTAL_LIGHTS = GRID_X * GRID_Z; // 8
        constexpr float DEFAULT_INTENSITY = 1.0f;
        constexpr float INTENSITY_MULTIPLIER = 500.0f;
    }

    // ===========================================
    // Ambient Lighting
    // ===========================================
    namespace Ambient {
        constexpr float DEFAULT_FILL = 2.0f;
        constexpr float MAX_FILL = 100.0f;
    }

    // ===========================================
    // Materials
    // ===========================================
    namespace Materials {
        // Room
        constexpr float ROOM_COLOR = 0.2f;
        constexpr float ROOM_SPECULAR = 0.8f;
        constexpr float ROOM_SHININESS = 64.0f;

        // Stage
        constexpr float STAGE_SPECULAR = 0.1f;
        constexpr float STAGE_SHININESS = 16.0f;
    }

    // ===========================================
    // Geometry Generation
    // ===========================================
    namespace Geometry {
        // Cone proxy
        constexpr int CONE_SEGMENTS = 16;
        constexpr float CONE_RADIUS = 1.0f;
        constexpr float CONE_HEIGHT = 1.0f;

        // Debug sphere
        constexpr int SPHERE_STACKS = 10;
        constexpr int SPHERE_SLICES = 10;
        constexpr float SPHERE_RADIUS = 2.0f;
    }

    // ===========================================
    // Post-Processing
    // ===========================================
    namespace PostProcess {
        // Blur
        constexpr int DEFAULT_BLUR_PASSES = 1;
        constexpr int MIN_BLUR_PASSES = 1;
        constexpr int MAX_BLUR_PASSES = 5;

        // Frame timing (approximate for 60fps)
        constexpr float FRAME_DELTA = 0.016f;
    }

    // ===========================================
    // UI Defaults
    // ===========================================
    namespace UI {
        constexpr float WINDOW_POS_X = 10.0f;
        constexpr float WINDOW_POS_Y = 10.0f;
        constexpr float WINDOW_WIDTH = 350.0f;
        constexpr float WINDOW_HEIGHT = 600.0f;
    }

    // ===========================================
    // Vertex Stride Constants
    // ===========================================
    namespace Vertex {
        // Position (3) + Normal (3) + UV (2) = 8 floats = 32 bytes
        constexpr unsigned int STRIDE_FULL = 32;
        // Position only (3) = 12 bytes
        constexpr unsigned int STRIDE_POSITION_ONLY = 12;
    }

} // namespace Config
