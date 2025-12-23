#include <DirectXMath.h>
#include <iostream>
#include <algorithm>
#include <cassert>

using namespace DirectX;

// Function to find intersection of ray with sphere
// Returns true if there is an intersection, and fills t0, t1
bool RaySphereIntersection(XMVECTOR rayOrigin, XMVECTOR rayDir, XMVECTOR sphereCenter, float sphereRadius, float& t0, float& t1) {
    XMVECTOR L = sphereCenter - rayOrigin;
    XMVECTOR tca = XMVector3Dot(L, rayDir);
    float d2 = XMVectorGetX(XMVector3Dot(L, L)) - XMVectorGetX(tca * tca);
    float r2 = sphereRadius * sphereRadius;
    if (d2 > r2) return false;
    float thc = sqrt(r2 - d2);
    t0 = XMVectorGetX(tca) - thc;
    t1 = XMVectorGetX(tca) + thc;
    return true;
}

int main() {
    XMVECTOR origin = XMVectorSet(0, 0, -10, 1);
    XMVECTOR dir = XMVectorSet(0, 0, 1, 0);
    XMVECTOR center = XMVectorSet(0, 0, 0, 1);
    float radius = 5.0f;

    float t0, t1;
    bool hit = RaySphereIntersection(origin, dir, center, radius, t0, t1);

    assert(hit);
    // Ray starts at -10, sphere is at 0 with radius 5.
    // Intersections should be at -5 and 5 relative to center, so 5 and 15 from origin.
    assert(std::abs(t0 - 5.0f) < 0.001f);
    assert(std::abs(t1 - 15.0f) < 0.001f);

    std::cout << "Ray-Sphere intersection test passed!" << std::endl;

    // Test miss
    dir = XMVectorSet(1, 0, 0, 0);
    hit = RaySphereIntersection(origin, dir, center, radius, t0, t1);
    assert(!hit);
    std::cout << "Ray-Sphere miss test passed!" << std::endl;

    return 0;
}
