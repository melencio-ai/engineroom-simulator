#pragma once

#include <SDL.h>

namespace enginewatch {

class BitmapFont {
public:
    explicit BitmapFont(SDL_Renderer* renderer);
    ~BitmapFont();
    BitmapFont(const BitmapFont&) = delete;
    BitmapFont& operator=(const BitmapFont&) = delete;

    void draw(SDL_Renderer* renderer, int x, int y, const char* text,
              SDL_Color color, int scale = 1) const;
    int width(const char* text, int scale = 1) const;
    bool available() const { return atlas_ != nullptr; }

private:
    SDL_Texture* atlas_ = nullptr;
};

} // namespace enginewatch
