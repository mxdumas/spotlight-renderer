#include "Renderer.h"
#include <cassert>
#include <iostream>

int main() {
    Renderer renderer;
    
    // Test default value
    float defaultWeight = renderer.GetTemporalWeight();
    if (defaultWeight < 0.0f || defaultWeight > 1.0f) {
        std::cerr << "Default temporal weight out of range!" << std::endl;
        return 1;
    }
    
    // Test Setter/Getter
    renderer.SetTemporalWeight(0.85f);
    if (abs(renderer.GetTemporalWeight() - 0.85f) > 0.001f) {
        std::cerr << "Failed to set temporal weight! Expected 0.85, got " << renderer.GetTemporalWeight() << std::endl;
        return 1;
    }

    // Test VolumetricBuffer size (params:16 + jitter:16 + prevViewProj:64 = 96)
    if (sizeof(VolumetricBuffer) != 96) {
        std::cerr << "VolumetricBuffer size mismatch! Expected 96, got " << sizeof(VolumetricBuffer) << std::endl;
        return 1;
    }

    std::cout << "Temporal Reconstruction Interface Test Passed" << std::endl;
    return 0;
}
