#pragma once

#include "scene/FontAtlas.hpp"
#include "scene/Game3D.hpp"
#include "scene/World3D.hpp"

#include <cstdint>
#include <vector>

namespace enginewatch {

class PspRenderer3D {
public:
    PspRenderer3D();
    ~PspRenderer3D();
    void draw(const World3D& world, const Game3D& game);

private:
    FontAtlas font_;
    bool ready_ = false;
    // Transient animated body/hands, never part of World3D static vertices.
    std::vector<Vertex3D> character_vertices_;

    void rectangle(int x, int y, int width, int height, uint32_t color);
    void text(int x, int y, const char* value, uint32_t color, int scale = 1);
    void overlay(const Hud3D& hud);
};

} // namespace enginewatch
