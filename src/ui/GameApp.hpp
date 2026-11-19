#pragma once

#include "core/EngineSimulation.hpp"
#include "ui/BitmapFont.hpp"

#include <SDL.h>

namespace enginewatch {

class GameApp {
public:
    explicit GameApp(SDL_Renderer* renderer);
    ~GameApp();
    void run();

private:
    enum Room { ControlRoom, EngineRoom } room_ = ControlRoom;
    enum Panel { NoPanel, PumpPanel, ConsolePanel, StrainerPanel, HelpPanel } panel_ = NoPanel;
    enum Action { Use, Back, Help, Pause, Restart, MenuUp, MenuDown };
    EngineSimulation sim_;
    SDL_Renderer* renderer_ = nullptr;
    BitmapFont font_;
    SDL_GameController* controller_ = nullptr;
    SDL_Joystick* joystick_ = nullptr;
    bool running_ = true;
    bool paused_ = false;
    int menu_index_ = 0;
    float player_x_ = 381.0f;
    float player_y_ = 205.0f;

    void handleEvent(const SDL_Event& event);
    void handleAction(Action action);
    void operateSelectedEquipment();
    void move(float dt);
    void changeRoom();
    bool closeTo(float x, float y, float max_distance) const;
    void render();
    void renderBackground();
    void renderControlRoom();
    void renderEngineRoom();
    void renderHud();
    void renderPanel();
    void renderPlayer();
    void text(int x, int y, const char* value, SDL_Color color, int scale=1);
    void fill(int x, int y, int w, int h, SDL_Color color);
    void outline(int x, int y, int w, int h, SDL_Color color);
    void line(int x1, int y1, int x2, int y2, SDL_Color color);
};

} // namespace enginewatch
