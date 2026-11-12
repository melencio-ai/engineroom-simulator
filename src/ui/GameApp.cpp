#include "ui/GameApp.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace enginewatch {

namespace {
const SDL_Color kWhite = {236, 246, 249, 255};
const SDL_Color kMute = {148, 181, 192, 255};
const SDL_Color kTeal = {103, 225, 198, 255};
const SDL_Color kOrange = {253, 190, 109, 255};
const SDL_Color kRed = {252, 117, 116, 255};
const SDL_Color kNavy = {9, 23, 34, 255};
const SDL_Color kPanel = {24, 45, 60, 255};
const SDL_Color kBorder = {66, 99, 111, 255};
const SDL_Color kFloor = {22, 48, 61, 255};

float axisValue(Sint16 value) {
    if (value > -7000 && value < 7000) return 0.0f;
    return static_cast<float>(value) / 32768.0f;
}
}

GameApp::GameApp(SDL_Renderer* renderer) : renderer_(renderer), font_(renderer) {
    for (int i = 0; i < SDL_NumJoysticks(); ++i) {
        if (SDL_IsGameController(i)) { controller_ = SDL_GameControllerOpen(i); break; }
    }
    if (!controller_ && SDL_NumJoysticks() > 0) joystick_ = SDL_JoystickOpen(0);
}

GameApp::~GameApp() {
    if (controller_) SDL_GameControllerClose(controller_);
    if (joystick_) SDL_JoystickClose(joystick_);
}

void GameApp::run() {
    Uint32 last = SDL_GetTicks();
    while (running_) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) handleEvent(event);
        const Uint32 now = SDL_GetTicks();
        const float dt = std::min(0.05f, static_cast<float>(now - last) / 1000.0f);
        last = now;
        if (!paused_) { sim_.step(dt); move(dt); }
        render();
        SDL_Delay(16); // PSP-friendly capped update cadence; simulation clamps dt independently.
    }
}

void GameApp::handleEvent(const SDL_Event& event) {
    if (event.type == SDL_QUIT) { running_ = false; return; }
    if (event.type == SDL_CONTROLLERDEVICEADDED && !controller_ && !joystick_) {
        controller_ = SDL_GameControllerOpen(event.cdevice.which);
    }
    if (event.type == SDL_CONTROLLERDEVICEREMOVED && controller_) {
        if (SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(controller_)) == event.cdevice.which) {
            SDL_GameControllerClose(controller_);
            controller_ = nullptr;
        }
    }
    if (event.type == SDL_KEYDOWN && event.key.repeat == 0) {
        const SDL_Keycode key = event.key.keysym.sym;
        if (key == SDLK_x || key == SDLK_e || key == SDLK_SPACE || key == SDLK_RETURN) handleAction(Use);
        if (key == SDLK_c || key == SDLK_ESCAPE || key == SDLK_BACKSPACE) handleAction(Back);
        if (key == SDLK_h || key == SDLK_t || key == SDLK_F1) handleAction(Help);
        if (key == SDLK_p || key == SDLK_TAB) handleAction(Pause);
        if (key == SDLK_r) handleAction(Restart);
    }
    if (event.type == SDL_CONTROLLERBUTTONDOWN) {
        switch (event.cbutton.button) {
        case SDL_CONTROLLER_BUTTON_A: handleAction(Use); break;       // PSP CROSS
        case SDL_CONTROLLER_BUTTON_B: handleAction(Back); break;      // PSP CIRCLE
        case SDL_CONTROLLER_BUTTON_Y: handleAction(Help); break;      // PSP TRIANGLE
        case SDL_CONTROLLER_BUTTON_START: handleAction(Pause); break;
        case SDL_CONTROLLER_BUTTON_BACK: handleAction(Help); break;
        default: break;
        }
    }
    // Raw PSP joystick fallback if the SDL GameController mapping is unavailable.
    if (event.type == SDL_JOYBUTTONDOWN && !controller_) {
        switch (event.jbutton.button) {
        case 2: handleAction(Use); break;      // Cross
        case 1: handleAction(Back); break;     // Circle
        case 0: handleAction(Help); break;     // Triangle
        case 11: handleAction(Pause); break;   // Start
        default: break;
        }
    }
}

void GameApp::handleAction(Action action) {
    if (action == Pause) { paused_ = !paused_; return; }
    if (action == Restart && paused_) {
        sim_.reset(); room_ = ControlRoom; panel_ = NoPanel;
        player_x_ = 381; player_y_ = 205; paused_ = false;
        return;
    }
    if (paused_) return;
    if (action == Help) { panel_ = (panel_ == HelpPanel) ? NoPanel : HelpPanel; return; }
    if (action == Back) { panel_ = NoPanel; return; }
    if (action != Use) return;

    if (panel_ == HelpPanel) { panel_ = NoPanel; return; }
    if (panel_ == PumpPanel) {
        sim_.requestLoPump(!sim_.state().lo_pump_requested);
        return;
    }
    if (panel_ == ConsolePanel) { sim_.inspectEcrConsole(); return; }
    if (room_ == ControlRoom) {
        if (closeTo(434, 200, 44)) { changeRoom(); return; }
        if (closeTo(223, 187, 90)) {
            panel_ = ConsolePanel;
            sim_.inspectEcrConsole();
        }
    } else {
        if (closeTo(53, 207, 51)) { changeRoom(); return; }
        if (closeTo(203, 168, 65)) panel_ = PumpPanel;
    }
}

void GameApp::changeRoom() {
    panel_ = NoPanel;
    if (room_ == ControlRoom) {
        room_ = EngineRoom;
        player_x_ = 85;
        player_y_ = 207;
    } else {
        room_ = ControlRoom;
        player_x_ = 387;
        player_y_ = 207;
    }
}

bool GameApp::closeTo(float x, float y, float max_distance) const {
    const float dx = player_x_ - x;
    const float dy = player_y_ - y;
    return dx * dx + dy * dy <= max_distance * max_distance;
}

void GameApp::move(float dt) {
    if (panel_ != NoPanel) return;
    float dx = 0.0f, dy = 0.0f;
    const Uint8* keys = SDL_GetKeyboardState(nullptr);
    if (keys[SDL_SCANCODE_LEFT] || keys[SDL_SCANCODE_A]) dx -= 1;
    if (keys[SDL_SCANCODE_RIGHT] || keys[SDL_SCANCODE_D]) dx += 1;
    if (keys[SDL_SCANCODE_UP] || keys[SDL_SCANCODE_W]) dy -= 1;
    if (keys[SDL_SCANCODE_DOWN] || keys[SDL_SCANCODE_S]) dy += 1;
    if (controller_) {
        if (SDL_GameControllerGetButton(controller_, SDL_CONTROLLER_BUTTON_DPAD_LEFT)) dx -= 1;
        if (SDL_GameControllerGetButton(controller_, SDL_CONTROLLER_BUTTON_DPAD_RIGHT)) dx += 1;
        if (SDL_GameControllerGetButton(controller_, SDL_CONTROLLER_BUTTON_DPAD_UP)) dy -= 1;
        if (SDL_GameControllerGetButton(controller_, SDL_CONTROLLER_BUTTON_DPAD_DOWN)) dy += 1;
        dx += axisValue(SDL_GameControllerGetAxis(controller_, SDL_CONTROLLER_AXIS_LEFTX));
        dy += axisValue(SDL_GameControllerGetAxis(controller_, SDL_CONTROLLER_AXIS_LEFTY));
    } else if (joystick_) {
        dx += axisValue(SDL_JoystickGetAxis(joystick_, 0));
        dy += axisValue(SDL_JoystickGetAxis(joystick_, 1));
        // PSP raw joystick mapping: directional buttons 6..9 on legacy implementations.
        if (SDL_JoystickGetButton(joystick_, 7)) dx -= 1;
        if (SDL_JoystickGetButton(joystick_, 9)) dx += 1;
        if (SDL_JoystickGetButton(joystick_, 8)) dy -= 1;
        if (SDL_JoystickGetButton(joystick_, 6)) dy += 1;
    }
    const float magnitude = std::sqrt(dx * dx + dy * dy);
    if (magnitude > 1.0f) { dx /= magnitude; dy /= magnitude; }
    player_x_ = std::max(23.0f, std::min(459.0f, player_x_ + dx * dt * 114.0f));
    player_y_ = std::max(46.0f, std::min(233.0f, player_y_ + dy * dt * 114.0f));
}

void GameApp::fill(int x, int y, int w, int h, SDL_Color color) {
    SDL_SetRenderDrawColor(renderer_, color.r, color.g, color.b, color.a);
    const SDL_Rect rect = {x,y,w,h};
    SDL_RenderFillRect(renderer_, &rect);
}
void GameApp::outline(int x, int y, int w, int h, SDL_Color color) {
    SDL_SetRenderDrawColor(renderer_, color.r, color.g, color.b, color.a);
    const SDL_Rect rect = {x,y,w,h};
    SDL_RenderDrawRect(renderer_, &rect);
}
void GameApp::line(int x1, int y1, int x2, int y2, SDL_Color color) {
    SDL_SetRenderDrawColor(renderer_, color.r, color.g, color.b, color.a);
    SDL_RenderDrawLine(renderer_, x1, y1, x2, y2);
}
void GameApp::text(int x, int y, const char* value, SDL_Color color, int scale) {
    font_.draw(renderer_, x, y, value, color, scale);
}

void GameApp::renderBackground() {
    fill(0, 0, 480, 272, kNavy);
    fill(0, 31, 480, 215, kFloor);
    // Deck grid makes the scene spatial rather than a static menu.
    for (int x = 17; x < 480; x += 27) line(x, 32, x, 247, kBorder);
    for (int y = 41; y < 247; y += 27) line(0, y, 480, y, kBorder);
}

void GameApp::renderControlRoom() {
    // ECR monitoring console.
    fill(25, 47, 318, 145, kPanel);
    outline(25, 47, 318, 145, kMute);
    fill(34, 55, 302, 14, kNavy);
    text(42, 59, "MACHINERY MONITOR - A", kTeal);
    text(37, 82, "ELECTRICAL BUS", kMute);
    fill(216, 80, 64, 12, kNavy);
    text(220, 83, sim_.state().electrical_bus_energized ? "ON-LINE" : "OFFLINE",
         sim_.state().electrical_bus_energized ? kTeal : kRed);
    text(37, 108, "LO PUMP STATUS", kMute);
    text(219, 108, sim_.state().lo_pump_running ? "RUNNING" :
         sim_.state().lo_pump_requested ? "STARTING" : "STOPPED",
         sim_.state().lo_pump_running ? kTeal : kOrange);
    text(37, 132, "LO PRESSURE", kMute);
    fill(149, 130, 148, 12, kNavy);
    int bar = static_cast<int>(std::min(4.5f, sim_.state().lo_pressure_bar) / 4.5f * 144.0f);
    fill(151, 132, bar, 8, sim_.state().lo_pressure_bar >= 3.0f ? kTeal : kOrange);
    char pressure[32];
    std::snprintf(pressure, sizeof(pressure), "%.1f BAR", sim_.state().lo_pressure_bar);
    text(226, 149, pressure, kWhite);
    text(38, 174, "X: INSPECT ECR CONSOLE", kTeal);
    // Exterior door toward the machinery platform.
    fill(403, 158, 54, 77, kPanel);
    outline(403, 158, 54, 77, kOrange);
    text(412, 173, "ER", kWhite, 2);
    text(409, 207, "DOOR", kOrange);
    text(36, 220, "ECR  >  ENGINE ROOM", kMute);
}

void GameApp::renderEngineRoom() {
    // Main engine silhouette and turbocharger casing.
    fill(294, 77, 150, 133, kPanel);
    outline(294, 77, 150, 133, kMute);
    for (int i = 0; i < 4; ++i) {
        fill(307 + i*31, 106, 23, 43, kBorder);
        outline(307 + i*31, 106, 23, 43, kMute);
    }
    text(308, 89, "MAIN ENGINE", kWhite);
    text(318, 167, "STOPPED", kOrange);
    // Pipes lead from pump to main engine.
    line(218, 164, 280, 164, kTeal);
    line(280, 164, 280, 131, kTeal);
    line(280, 131, 294, 131, kTeal);
    line(218, 167, 281, 167, kTeal);
    // Local lubricating oil pump, motor and status lamp.
    fill(169, 124, 72, 79, kPanel);
    outline(169, 124, 72, 79, kTeal);
    fill(183, 141, 35, 33, kBorder);
    outline(183, 141, 35, 33, kWhite);
    fill(193, 151, 15, 13, sim_.state().lo_pump_running ? kTeal : kOrange);
    text(177, 185, "LO PUMP", kWhite);
    text(158, 111, "LOCAL STATION", kMute);
    // Pipe supply tank, schematic only.
    fill(108, 70, 71, 42, kPanel);
    outline(108, 70, 71, 42, kMute);
    text(115, 81, "LO SUMP", kMute);
    line(143, 112, 143, 164, kTeal);
    line(143, 164, 168, 164, kTeal);
    // Door to control room.
    fill(22, 181, 59, 53, kPanel);
    outline(22, 181, 59, 53, kOrange);
    text(36, 192, "ECR", kWhite, 2);
    text(29, 219, "DOOR", kOrange);
    text(97, 224, "PLATFORM 1 - LUBE OIL", kMute);
}

void GameApp::renderPlayer() {
    // High-contrast avatar and small direction marker for 480x272 displays.
    const int x = static_cast<int>(player_x_);
    const int y = static_cast<int>(player_y_);
    fill(x-6, y-5, 13, 14, kNavy);
    outline(x-6, y-5, 13, 14, kWhite);
    fill(x-3, y-8, 7, 7, kTeal);
    fill(x-4, y+2, 9, 6, kTeal);
}

void GameApp::renderHud() {
    fill(0, 0, 480, 31, kNavy);
    text(15, 8, "ENGINE WATCH", kWhite, 2);
    text(283, 7, "MISSION 001", kOrange);
    text(283, 19, room_ == ControlRoom ? "ENGINE CONTROL ROOM" : "ENGINE ROOM - DECK 1", kMute);
    fill(0, 246, 480, 26, kNavy);
    text(12, 250, "MOVE: D-PAD / STICK   X: USE   O: BACK", kWhite);
    text(12, 260, "TRIANGLE: HELP   START: PAUSE", kMute);
    if (sim_.state().objective_complete) {
        fill(31, 32, 417, 15, kTeal);
        text(41, 36, "MISSION COMPLETE: LO PUMP VERIFIED IN ECR", kNavy);
    } else {
        text(11, 236, room_ == ControlRoom ?
             (closeTo(434, 200, 44) ? "X: ENTER ENGINE ROOM" :
              closeTo(223, 187, 90) ? "X: INSPECT ECR CONSOLE" :
              "GO TO ENGINE ROOM AND START LO PUMP") :
             (closeTo(53, 207, 51) ? "X: RETURN TO ECR" :
              closeTo(203, 168, 65) ? "X: OPERATE LOCAL LO PUMP" :
              "APPROACH THE LO PUMP FOR LOCAL CONTROLS"), kTeal);
    }
}

void GameApp::renderPanel() {
    if (panel_ == NoPanel && !paused_) return;
    fill(73, 45, 336, 190, kNavy);
    outline(73, 45, 336, 190, kTeal);
    fill(84, 57, 314, 21, kPanel);
    if (paused_) {
        text(99, 64, "SIMULATION PAUSED", kOrange, 2);
        text(100, 113, "START : RESUME", kWhite);
        text(100, 137, "R : RESET MISSION (DESKTOP)", kMute);
        text(100, 185, "CURRENT STATE IS NOT ADVANCING", kTeal);
        return;
    }
    if (panel_ == HelpPanel) {
        text(99, 64, "ENGINE WATCH - CONTROLS", kWhite);
        text(100, 95, "D-PAD / STICK: WALK", kMute);
        text(100, 115, "CROSS (X): USE / TOGGLE", kMute);
        text(100, 135, "CIRCLE (O): CLOSE PANEL", kMute);
        text(100, 155, "TRIANGLE: HELP", kMute);
        text(100, 175, "START: PAUSE", kMute);
        text(100, 211, "CROSS OR TRIANGLE TO CLOSE", kTeal);
        return;
    }
    char pressure[48];
    std::snprintf(pressure, sizeof(pressure), "LO PRESSURE: %.2f BAR", sim_.state().lo_pressure_bar);
    if (panel_ == PumpPanel) {
        text(99, 64, "LOCAL LUBE OIL PUMP", kWhite);
        text(100, 96, sim_.state().lo_pump_running ? "MOTOR: RUNNING" :
             sim_.state().lo_pump_requested ? "MOTOR: STARTING" : "MOTOR: STOPPED",
             sim_.state().lo_pump_running ? kTeal : kOrange);
        text(100, 118, pressure, kWhite);
        text(100, 143, "POWER: 440V BUS A - READY", kMute);
        fill(97, 171, 288, 28, kPanel);
        text(110, 180, sim_.state().lo_pump_requested ? "X: STOP PUMP" : "X: START PUMP", kTeal, 2);
        text(100, 217, "O: CLOSE LOCAL CONTROL", kMute);
    } else if (panel_ == ConsolePanel) {
        text(99, 64, "ECR - LO SYSTEM MONITOR", kWhite);
        text(100, 93, pressure, kWhite);
        text(100, 116, sim_.state().lo_pump_running ? "PUMP: RUNNING" : "PUMP: STOPPED",
             sim_.state().lo_pump_running ? kTeal : kOrange);
        text(100, 139, sim_.state().lo_pressure_alarm ? "ALARM: LO PRESSURE LOW" : "ALARMS: NORMAL",
             sim_.state().lo_pressure_alarm ? kRed : kTeal);
        text(100, 164, sim_.state().objective_complete ? "MISSION: VERIFIED" :
             "MISSION: PENDING VERIFICATION", kOrange);
        fill(97, 188, 288, 22, kPanel);
        text(107, 195, "X: VERIFY   O: CLOSE", kTeal);
    }
}

void GameApp::render() {
    SDL_SetRenderDrawBlendMode(renderer_, SDL_BLENDMODE_NONE);
    renderBackground();
    if (room_ == ControlRoom) renderControlRoom(); else renderEngineRoom();
    renderPlayer();
    renderHud();
    renderPanel();
    SDL_RenderPresent(renderer_);
}

} // namespace enginewatch
