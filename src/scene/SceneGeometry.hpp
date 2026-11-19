#pragma once

#include <cstdint>

namespace enginewatch {

// Shared PSP-native geometry types. These stay minimal and are reused by
// procedural equipment, deck/stair layout, and GU/desktop renderers.
struct Vec3 {
    float x, y, z;
};
struct Vertex3D {
    uint32_t color; // PSP ABGR8888 vertex colour
    float x, y, z;
};
} // namespace enginewatch
