#include "Rendering/Passes/VolumetricPass.h"
#include <cassert>
#include <iostream>

int main() {
    // We expect VolumetricBuffer to be 32 bytes (2 x float4)
    // Currently it is 16 bytes (1 x float4)
    // This test ensures we have added the necessary padding/data for jitter
    if (sizeof(VolumetricBuffer) != 32) {
        std::cerr << "VolumetricBuffer size mismatch! Expected 32, got " << sizeof(VolumetricBuffer) << std::endl;
        return 1;
    }
    std::cout << "Test Passed" << std::endl;
    return 0;
}
