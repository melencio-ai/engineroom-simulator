#pragma once

#include <cstdint>
#include <vector>

namespace enginewatch {

// World units are metres; X/Z is the floor plane and Y is height.
// Vertex colours use the PSP GU ABGR8888 layout.
struct Vec3 {
    float x;
    float y;
    float z;
};

struct Vertex3D {
    uint32_t color;
    float x;
    float y;
    float z;
};

struct Footprint {
    float x0;
    float x1;
    float z0;
    float z1;
};

enum StationId {
    NoStation = 0,
    EcrConsoleStation = 1,
    LoPumpStation = 2,
    StrainerStation = 3
};

class World3D {
public:
    World3D();
    const std::vector<Vertex3D>& vertices() const { return vertices_; }
    bool canStand(float x, float z, float radius = 0.30f) const;
    StationId lookAtStation(const Vec3& camera, float yaw) const;
    bool isInEcr(float x) const { return x < -3.0f; }

private:
    std::vector<Vertex3D> vertices_;
    std::vector<Footprint> obstacles_;

    void triangle(const Vec3& a, const Vec3& b, const Vec3& c, uint32_t color);
    void quad(const Vec3& a, const Vec3& b, const Vec3& c, const Vec3& d, uint32_t color);
    void box(float cx, float cy, float cz, float hx, float hy, float hz, uint32_t color);
    void cylinderX(float x0, float x1, float y, float z, float radius, int segments, uint32_t color);
    void cylinderY(float x, float y0, float y1, float z, float radius, int segments, uint32_t color);
    void pipeZ(float z0, float z1, float x, float y, float radius, uint32_t color);
    void obstacle(float x0, float x1, float z0, float z1);
    void build();
};

} // namespace enginewatch
