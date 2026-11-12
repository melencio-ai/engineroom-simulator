#include "ui/BitmapFont.hpp"

#include <cctype>
#include <cstring>
#include <cstdint>

namespace enginewatch {

namespace {
struct Glyph { char character; unsigned char rows[7]; };
// Compact, built-in 5x7 uppercase font: no external files or runtime font dependencies.
const Glyph glyphs[] = {
    {'!', {4,4,4,4,4,0,4}},
    {'"', {10,10,10,0,0,0,0}},
    {'#', {10,31,10,10,31,10,0}},
    {'$', {14,21,20,14,5,21,14}},
    {'%', {17,2,4,4,8,17,0}},
    {'&', {12,18,20,8,21,18,13}},
    {'\'', {4,4,4,0,0,0,0}},
    {'(', {2,4,8,8,8,4,2}},
    {')', {8,4,2,2,2,4,8}},
    {'*', {0,21,14,31,14,21,0}},
    {'+', {0,4,4,31,4,4,0}},
    {',', {0,0,0,0,4,4,8}},
    {'-', {0,0,0,31,0,0,0}},
    {'.', {0,0,0,0,0,12,12}},
    {'/', {1,2,2,4,8,8,16}},
    {'0', {14,17,19,21,25,17,14}},
    {'1', {4,12,4,4,4,4,14}},
    {'2', {14,17,1,2,4,8,31}},
    {'3', {30,1,1,14,1,1,30}},
    {'4', {2,6,10,18,31,2,2}},
    {'5', {31,16,30,1,1,17,14}},
    {'6', {6,8,16,30,17,17,14}},
    {'7', {31,1,2,4,8,8,8}},
    {'8', {14,17,17,14,17,17,14}},
    {'9', {14,17,17,15,1,2,12}},
    {':', {0,4,4,0,4,4,0}},
    {';', {0,4,4,0,4,4,8}},
    {'<', {2,4,8,16,8,4,2}},
    {'=', {0,0,31,0,31,0,0}},
    {'>', {8,4,2,1,2,4,8}},
    {'?', {14,17,1,2,4,0,4}},
    {'@', {14,17,23,21,23,16,14}},
    {'A', {14,17,17,31,17,17,17}},
    {'B', {30,17,17,30,17,17,30}},
    {'C', {14,17,16,16,16,17,14}},
    {'D', {28,18,17,17,17,18,28}},
    {'E', {31,16,16,30,16,16,31}},
    {'F', {31,16,16,30,16,16,16}},
    {'G', {14,17,16,23,17,17,14}},
    {'H', {17,17,17,31,17,17,17}},
    {'I', {14,4,4,4,4,4,14}},
    {'J', {7,2,2,2,18,18,12}},
    {'K', {17,18,20,24,20,18,17}},
    {'L', {16,16,16,16,16,16,31}},
    {'M', {17,27,21,21,17,17,17}},
    {'N', {17,25,21,19,17,17,17}},
    {'O', {14,17,17,17,17,17,14}},
    {'P', {30,17,17,30,16,16,16}},
    {'Q', {14,17,17,17,21,18,13}},
    {'R', {30,17,17,30,20,18,17}},
    {'S', {15,16,16,14,1,1,30}},
    {'T', {31,4,4,4,4,4,4}},
    {'U', {17,17,17,17,17,17,14}},
    {'V', {17,17,17,17,17,10,4}},
    {'W', {17,17,17,21,21,21,10}},
    {'X', {17,17,10,4,10,17,17}},
    {'Y', {17,17,10,4,4,4,4}},
    {'Z', {31,1,2,4,8,16,31}},
    {'[', {14,8,8,8,8,8,14}},
    {'\\', {16,8,8,4,2,2,1}},
    {']', {14,2,2,2,2,2,14}},
    {'^', {4,10,17,0,0,0,0}},
    {'_', {0,0,0,0,0,0,31}},
};
const unsigned char empty[7] = {0,0,0,0,0,0,0};
const unsigned char* rowsFor(unsigned char character) {
    if (character >= 'a' && character <= 'z') character -= 'a' - 'A';
    for (unsigned i = 0; i < sizeof(glyphs) / sizeof(glyphs[0]); ++i) {
        if (static_cast<unsigned char>(glyphs[i].character) == character) return glyphs[i].rows;
    }
    return empty;
}
}

BitmapFont::BitmapFont(SDL_Renderer* renderer) {
    SDL_Surface* atlas = SDL_CreateRGBSurfaceWithFormat(0, 128, 32, 32, SDL_PIXELFORMAT_RGBA32);
    if (!atlas) return;
    SDL_FillRect(atlas, nullptr, SDL_MapRGBA(atlas->format, 0, 0, 0, 0));
    if (SDL_LockSurface(atlas) != 0) { SDL_FreeSurface(atlas); return; }
    const Uint32 white = SDL_MapRGBA(atlas->format, 255, 255, 255, 255);
    for (int ch = 32; ch < 96; ++ch) {
        const unsigned char* rows = rowsFor(static_cast<unsigned char>(ch));
        const int sx = ((ch - 32) % 16) * 6;
        const int sy = ((ch - 32) / 16) * 8;
        for (int y = 0; y < 7; ++y) {
            for (int x = 0; x < 5; ++x) {
                if (rows[y] & (1 << (4-x))) {
                    Uint32* pixel = reinterpret_cast<Uint32*>(static_cast<unsigned char*>(atlas->pixels)
                        + (sy + y) * atlas->pitch + (sx + x) * sizeof(Uint32));
                    *pixel = white;
                }
            }
        }
    }
    SDL_UnlockSurface(atlas);
    atlas_ = SDL_CreateTextureFromSurface(renderer, atlas);
    SDL_FreeSurface(atlas);
    if (atlas_) SDL_SetTextureBlendMode(atlas_, SDL_BLENDMODE_BLEND);
}

BitmapFont::~BitmapFont() { if (atlas_) SDL_DestroyTexture(atlas_); }

void BitmapFont::draw(SDL_Renderer* renderer, int x, int y, const char* text,
                      SDL_Color color, int scale) const {
    if (!atlas_ || !text || scale <= 0) return;
    SDL_SetTextureColorMod(atlas_, color.r, color.g, color.b);
    SDL_SetTextureAlphaMod(atlas_, color.a);
    const int beginX = x;
    for (const unsigned char* ch = reinterpret_cast<const unsigned char*>(text); *ch; ++ch) {
        if (*ch == '\n') { y += 9 * scale; x = beginX; continue; }
        unsigned char c = *ch;
        if (c >= 'a' && c <= 'z') c -= 'a' - 'A';
        if (c < 32 || c >= 96) c = '?';
        const SDL_Rect source = {((c - 32) % 16) * 6, ((c - 32) / 16) * 8, 5, 7};
        const SDL_Rect destination = {x, y, 5 * scale, 7 * scale};
        SDL_RenderCopy(renderer, atlas_, &source, &destination);
        x += 6 * scale;
    }
}

int BitmapFont::width(const char* text, int scale) const {
    if (!text) return 0;
    return static_cast<int>(std::strlen(text)) * 6 * scale;
}

} // namespace enginewatch
