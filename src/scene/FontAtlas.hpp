#pragma once

#include <cstdint>

namespace enginewatch {

// Tiny 5x7 bitmap glyphs shared by PSP GU and the optional desktop preview.
// Exactly 128x32, RGBA8888, transparent outside the glyph pixels.
class FontAtlas {
public:
    enum { Width = 128, Height = 32 };
    FontAtlas();
    const uint32_t* pixels() const { return pixels_; }
private:
    alignas(16) uint32_t pixels_[Width * Height];
};

} // namespace enginewatch
