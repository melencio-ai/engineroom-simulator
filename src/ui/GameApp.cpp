#include "ui/GameApp.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace enginewatch {

namespace {
const SDL_Color kWhite  = {236, 246, 249, 255};
const SDL_Color kMute   = {148, 181, 192, 255};
const SDL_Color kTeal   = {103, 225, 198, 255};
const SDL_Color kOrange = {253, 190, 109, 255};
const SDL_Color kRed    = {252, 117, 116, 255};
const SDL_Color kNavy   = {9, 23, 34, 255};
const SDL_Color kPanel  = {24, 45, 60, 255};
const SDL_Color kBorder = {66, 99, 111, 255};
const SDL_Color kFloor  = {22, 48, 61, 255};

float axisValue(Sint16 value) {
    if (value > -7000 && value < 7000) return 0.0f;
    return static_cast<float>(value) / 32768.0f;
}
const char* motorStatus(const PumpState& p) {
    return p.fault ? "FAULT" : p.running ? "RUNNING" : p.requested ? "STARTING" : "STOPPED";
}
const char* valveStatus(bool open) { return open ? "OPEN" : "CLOSED"; }
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
        SDL_Delay(16);
    }
}

void GameApp::handleEvent(const SDL_Event& event) {
    if (event.type == SDL_QUIT) { running_ = false; return; }
    if (event.type == SDL_CONTROLLERDEVICEADDED && !controller_ && !joystick_)
        controller_ = SDL_GameControllerOpen(event.cdevice.which);
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
        if (key == SDLK_UP || key == SDLK_w) handleAction(MenuUp);
        if (key == SDLK_DOWN || key == SDLK_s) handleAction(MenuDown);
    }
    if (event.type == SDL_CONTROLLERBUTTONDOWN) {
        switch (event.cbutton.button) {
        case SDL_CONTROLLER_BUTTON_A: handleAction(Use); break;
        case SDL_CONTROLLER_BUTTON_B: handleAction(Back); break;
        case SDL_CONTROLLER_BUTTON_Y: handleAction(Help); break;
        case SDL_CONTROLLER_BUTTON_START: handleAction(Pause); break;
        case SDL_CONTROLLER_BUTTON_DPAD_UP: handleAction(MenuUp); break;
        case SDL_CONTROLLER_BUTTON_DPAD_DOWN: handleAction(MenuDown); break;
        default: break;
        }
    }
    if (event.type == SDL_JOYBUTTONDOWN && !controller_) {
        switch (event.jbutton.button) {
        case 2: handleAction(Use); break;          // CROSS
        case 1: handleAction(Back); break;         // CIRCLE
        case 0: handleAction(Help); break;         // TRIANGLE
        case 11: handleAction(Pause); break;       // START
        case 6: handleAction(MenuUp); break;       // PSP D-PAD UP
        case 8: handleAction(MenuDown); break;     // PSP D-PAD DOWN
        default: break;
        }
    }
}

void GameApp::handleAction(Action action) {
    if (action == Pause) { paused_ = !paused_; return; }
    if (paused_) {
        if (action == Restart || action == Use) {
            sim_.reset(); room_ = ControlRoom; panel_ = NoPanel; menu_index_ = 0;
            player_x_ = 381; player_y_ = 205; paused_ = false;
        }
        return;
    }
    if (action == Restart) return;
    if (action == Help) { panel_ = (panel_ == HelpPanel) ? NoPanel : HelpPanel; return; }
    if (action == Back) { panel_ = NoPanel; return; }
    if (action == MenuUp || action == MenuDown) {
        if (panel_ == PumpPanel) {
            const int offset = action == MenuUp ? -1 : 1;
            menu_index_ = (menu_index_ + offset + 6) % 6;
        }
        return;
    }
    if (action != Use) return;
    if (panel_ == HelpPanel || panel_ == StrainerPanel) { panel_ = NoPanel; return; }
    if (panel_ == PumpPanel) { operateSelectedEquipment(); return; }
    if (panel_ == ConsolePanel) {
        const MachineryState& s = sim_.state();
        if (s.mission == 1) {
            if (s.objective_complete) { sim_.beginMissionTwo(); panel_ = NoPanel; }
            else sim_.inspectEcrConsole();
        } else {
            if (s.lo_pressure_alarm && !s.alarm_acknowledged) sim_.acknowledgeAlarm();
            else sim_.verifyMissionTwo();
        }
        return;
    }
    if (room_ == ControlRoom) {
        if (closeTo(434, 200, 44)) { changeRoom(); return; }
        if (closeTo(223, 187, 90)) { panel_ = ConsolePanel; return; }
    } else {
        if (closeTo(53, 207, 51)) { changeRoom(); return; }
        if (closeTo(124, 110, 46)) { sim_.inspectStrainer(); panel_ = StrainerPanel; return; }
        if (closeTo(203, 174, 76)) { panel_ = PumpPanel; menu_index_ = 0; return; }
    }
}

void GameApp::operateSelectedEquipment() {
    const MachineryState& s = sim_.state();
    switch (menu_index_) {
    case 0: sim_.requestPump(MainLoPump, !s.main_pump.requested); break;
    case 1: sim_.requestPump(StandbyLoPump, !s.standby_pump.requested); break;
    case 2: sim_.setValve(MainLoPump, SuctionValve, !s.main_pump.suction_open); break;
    case 3: sim_.setValve(MainLoPump, DischargeValve, !s.main_pump.discharge_open); break;
    case 4: sim_.setValve(StandbyLoPump, SuctionValve, !s.standby_pump.suction_open); break;
    case 5: sim_.setValve(StandbyLoPump, DischargeValve, !s.standby_pump.discharge_open); break;
    default: break;
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
    for (int x = 17; x < 480; x += 27) line(x, 32, x, 247, kBorder);
    for (int y = 41; y < 247; y += 27) line(0, y, 480, y, kBorder);
}

void GameApp::renderControlRoom() {
    const MachineryState& s = sim_.state();
    fill(25, 49, 318, 148, kPanel);
    outline(25, 49, 318, 148, kMute);
    fill(34, 56, 302, 16, kNavy);
    text(42, 60, "LO SYSTEM MONITOR - A", kTeal);
    text(37, 80, "ELECTRICAL BUS", kMute);
    text(210, 80, s.electrical_bus_energized ? "ON-LINE" : "OFFLINE",
        s.electrical_bus_energized ? kTeal : kRed);
    text(37, 99, "MAIN LO PUMP", kMute);
    text(210, 99, motorStatus(s.main_pump), s.main_pump.running ? kTeal : kOrange);
    text(37, 116, "STBY LO PUMP", kMute);
    text(210, 116, motorStatus(s.standby_pump), s.standby_pump.running ? kTeal : kOrange);
    text(37, 136, "LO HEADER", kMute);
    fill(146, 135, 146, 11, kNavy);
    const int bar = static_cast<int>(s.lo_pressure_bar / 4.4f * 140.0f);
    fill(149, 138, bar, 5, s.lo_pressure_bar >= 3.0f ? kTeal : kRed);
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%.2f BAR  /  %.0f C", s.lo_pressure_bar, s.lo_oil_temp_c);
    text(144, 151, buf, kWhite);
    text(37, 173, s.lo_pressure_alarm ? (s.alarm_acknowledged ? "LO PRESS LOW - ACK" : "LO PRESS LOW - ACTIVE") : "ALARMS: NORMAL",
         s.lo_pressure_alarm ? kRed : kTeal);
    text(37, 186, "X: OPEN CONSOLE", kTeal);
    // Navigation to the machinery platform.
    fill(403, 158, 54, 77, kPanel);
    outline(403, 158, 54, 77, kOrange);
    text(412, 173, "ER", kWhite, 2);
    text(409, 207, "DOOR", kOrange);
}

void GameApp::renderEngineRoom() {
    const MachineryState& s = sim_.state();
    // Main engine silhouette. The machine is not operable in this milestone.
    fill(299, 76, 146, 137, kPanel);
    outline(299, 76, 146, 137, kMute);
    text(313, 88, "MAIN ENGINE", kWhite);
    for (int i = 0; i < 4; ++i) {
        fill(315 + i*29, 110, 23, 45, kBorder);
        outline(315 + i*29, 110, 23, 45, kMute);
    }
    text(320, 177, "STOPPED", kOrange);
    // Sump and branch to the pump station.
    fill(99, 70, 90, 33, kPanel);
    outline(99, 70, 90, 33, kMute);
    text(109, 81, "LO SUMP", kMute);
    line(147, 103, 147, 127, kTeal);
    // Main suction strainer, turns red when differential pressure is high.
    const SDL_Color strainer = s.main_strainer_dp_bar > 1.0f ? kRed : kOrange;
    fill(105, 109, 42, 28, kPanel);
    outline(105, 109, 42, 28, strainer);
    text(108, 118, "STR", strainer);
    text(92, 144, "X: INSPECT", kMute);
    line(148, 126, 168, 126, kTeal);
    line(168, 126, 168, 153, kTeal);
    // Two parallel pumps are controlled from one local station.
    fill(151, 153, 128, 60, kPanel);
    outline(151, 153, 128, 60, kTeal);
    fill(164, 167, 40, 24, kBorder);
    fill(223, 167, 40, 24, kBorder);
    fill(177, 176, 12, 9, s.main_pump.running ? kTeal : kOrange);
    fill(237, 176, 12, 9, s.standby_pump.running ? kTeal : kOrange);
    text(161, 196, "MAIN", kWhite);
    text(223, 196, "STBY", kWhite);
    text(165, 218, "LOCAL PUMP STATION", kMute);
    line(278, 171, 290, 171, kTeal);
    line(290, 171, 290, 137, kTeal);
    line(290, 137, 299, 137, kTeal);
    // Door to ECR.
    fill(22, 181, 59, 53, kPanel);
    outline(22, 181, 59, 53, kOrange);
    text(36, 192, "ECR", kWhite, 2);
    text(29, 219, "DOOR", kOrange);
}

void GameApp::renderPlayer() {
    const int x = static_cast<int>(player_x_);
    const int y = static_cast<int>(player_y_);
    fill(x - 6, y - 5, 13, 14, kNavy);
    outline(x - 6, y - 5, 13, 14, kWhite);
    fill(x - 3, y - 8, 7, 7, kTeal);
    fill(x - 4, y + 2, 9, 6, kTeal);
}

void GameApp::renderHud() {
    const MachineryState& s = sim_.state();
    fill(0, 0, 480, 31, kNavy);
    text(15, 8, "ENGINE WATCH", kWhite, 2);
    text(289, 7, s.mission == 1 ? "MISSION 001" : "MISSION 002", kOrange);
    text(289, 19, room_ == ControlRoom ? "ENGINE CONTROL ROOM" : "ENGINE ROOM - DECK 1", kMute);

    // Put the actual mission goal beneath the header, away from touch controls.
    fill(0, 31, 480, 15, kNavy);
    const char* goal = "";
    if (s.mission == 1) {
        goal = s.objective_complete ? "001 PASSED - X AT ECR TO CONTINUE" :
            s.lo_pressure_bar >= 3.0f ? "LO NORMAL: RETURN TO ECR AND VERIFY" :
            "001 START MAIN LO PUMP LOCALLY";
    } else if (s.mission_two_complete) {
        goal = "002 PASSED - HEADER RESTORED ON STANDBY";
    } else {
        goal = s.main_strainer_dp_bar > 1.0f ?
            "002 HIGH STRAINER DP - TRANSFER TO STANDBY" :
            "002 INVESTIGATE FALLING LO PRESSURE";
    }
    text(20, 35, goal, s.mission_two_complete ? kTeal : kOrange);

    // A short contextual prompt replaces v0.1's crowded two-line footer.
    // The 220px band sits above PPSSPP Android virtual SELECT/START buttons.
    fill(104, 220, 270, 14, kNavy);
    const char* prompt = "TRIANGLE HELP   X USE   O BACK";
    if (panel_ != NoPanel) prompt = "D-PAD SELECT   X OPERATE   O CLOSE";
    else if (room_ == ControlRoom) {
        if (closeTo(434, 200, 44)) prompt = "X: ENTER ENGINE ROOM";
        else if (closeTo(223, 187, 90)) prompt = "X: INSPECT ECR CONSOLE";
    } else {
        if (closeTo(53, 207, 51)) prompt = "X: RETURN TO ECR";
        else if (closeTo(124, 110, 46)) prompt = "X: INSPECT MAIN STRAINER";
        else if (closeTo(203, 174, 76)) prompt = "X: LOCAL PUMP CONTROLS";
    }
    text(115, 223, prompt, kWhite);
    if (s.mission_two_complete) {
        fill(44, 49, 396, 16, kTeal);
        text(58, 53, "MISSION 002 COMPLETE - LO PRESSURE RESTORED", kNavy);
    } else if (s.objective_complete && s.mission == 1) {
        fill(44, 49, 396, 16, kTeal);
        text(60, 53, "MISSION 001 COMPLETE - VERIFIED IN ECR", kNavy);
    }
}

void GameApp::renderPanel() {
    if (panel_ == NoPanel && !paused_) return;
    fill(69, 47, 342, 188, kNavy);
    outline(69, 47, 342, 188, kTeal);
    fill(80, 57, 320, 20, kPanel);
    if (paused_) {
        text(94, 63, "SIMULATION PAUSED", kOrange, 2);
        text(95, 114, "START : RESUME", kWhite);
        text(95, 137, "X : RESTART ALL MISSIONS", kMute);
        text(95, 180, "MACHINERY STATES ARE FROZEN", kTeal);
        return;
    }
    if (panel_ == HelpPanel) {
        text(94, 63, "ENGINE WATCH - CONTROLS", kWhite);
        text(95, 94, "D-PAD / STICK: WALK", kMute);
        text(95, 113, "D-PAD UP/DOWN: SELECT IN PANEL", kMute);
        text(95, 132, "X: INTERACT / OPERATE", kMute);
        text(95, 151, "O: CLOSE PANEL", kMute);
        text(95, 170, "START: PAUSE  THEN X: RESET", kMute);
        text(95, 209, "TRIANGLE OR X: CLOSE HELP", kTeal);
        return;
    }
    const MachineryState& s = sim_.state();
    char number[64];
    if (panel_ == PumpPanel) {
        text(92, 63, "LOCAL LO PUMP & VALVE CONTROLS", kWhite);
        std::snprintf(number, sizeof(number), "HEADER %.2f BAR    LO TEMP %.0f C", s.lo_pressure_bar, s.lo_oil_temp_c);
        text(91, 82, number, kMute);
        const char* labels[6] = {
            "MAIN PUMP MOTOR", "STBY PUMP MOTOR", "MAIN SUCTION", "MAIN DISCHARGE",
            "STBY SUCTION", "STBY DISCHARGE"
        };
        const char* values[6] = {
            motorStatus(s.main_pump), motorStatus(s.standby_pump),
            valveStatus(s.main_pump.suction_open), valveStatus(s.main_pump.discharge_open),
            valveStatus(s.standby_pump.suction_open), valveStatus(s.standby_pump.discharge_open)
        };
        for (int i = 0; i < 6; ++i) {
            const int y = 99 + i * 18;
            if (i == menu_index_) fill(82, y - 3, 315, 17, kPanel);
            text(89, y, i == menu_index_ ? ">" : " ", i == menu_index_ ? kOrange : kMute);
            text(104, y, labels[i], i == menu_index_ ? kWhite : kMute);
            const bool good = (i == 0 ? s.main_pump.running : i == 1 ? s.standby_pump.running :
                i == 2 ? s.main_pump.suction_open : i == 3 ? s.main_pump.discharge_open :
                i == 4 ? s.standby_pump.suction_open : s.standby_pump.discharge_open);
            text(309, y, values[i], good ? kTeal : kOrange);
        }
        text(95, 215, "UP/DOWN: SELECT   X: TOGGLE   O: EXIT", kTeal);
    } else if (panel_ == StrainerPanel) {
        text(94, 63, "MAIN LO SUCTION STRAINER", kWhite);
        std::snprintf(number, sizeof(number), "STRAINER DP: %.2f BAR", s.main_strainer_dp_bar);
        text(94, 94, number, s.main_strainer_dp_bar > 1.0f ? kRed : kWhite);
        std::snprintf(number, sizeof(number), "MAIN SUCTION: %.2f BAR", s.main_pump.suction_pressure_bar);
        text(94, 117, number, kWhite);
        text(94, 141, s.main_strainer_dp_bar > 1.0f ? "HIGH DP: SUCTION RESTRICTION" :
             "NO HIGH DIFFERENTIAL PRESSURE", s.main_strainer_dp_bar > 1.0f ? kRed : kTeal);
        text(94, 165, s.strainer_inspected ? "EVIDENCE RECORDED" : "OBSERVE WITH MAIN PUMP RUNNING", kOrange);
        text(95, 215, "X / O: CLOSE INSPECTION", kTeal);
    } else if (panel_ == ConsolePanel) {
        text(93, 63, "ECR - LO SYSTEM MONITOR", kWhite);
        std::snprintf(number, sizeof(number), "LO HEADER: %.2f BAR", s.lo_pressure_bar);
        text(94, 89, number, s.lo_pressure_bar >= 3.5f ? kTeal : kOrange);
        text(94, 110, "MAIN", kMute);
        text(144, 110, motorStatus(s.main_pump), s.main_pump.running ? kTeal : kOrange);
        text(246, 110, "STBY", kMute);
        text(289, 110, motorStatus(s.standby_pump), s.standby_pump.running ? kTeal : kOrange);
        std::snprintf(number, sizeof(number), "MAIN STRAINER DP: %.2f BAR", s.main_strainer_dp_bar);
        text(94, 130, number, s.main_strainer_dp_bar > 1.0f ? kRed : kWhite);
        text(94, 150, s.lo_pressure_alarm ?
            (s.alarm_acknowledged ? "ALARM: LO PRESS LOW (ACK)" : "ALARM: LO PRESS LOW - ACTIVE") :
            "ALARMS: NORMAL", s.lo_pressure_alarm ? kRed : kTeal);
        if (s.mission == 1) {
            text(94, 172, s.objective_complete ? "MISSION 001: VERIFIED" : "MISSION 001: NOT VERIFIED", kOrange);
            text(94, 213, s.objective_complete ? "X: START MISSION 002    O: CLOSE" :
                 "X: VERIFY LO PRESSURE  O: CLOSE", kTeal);
        } else {
            text(94, 172, s.mission_two_complete ? "MISSION 002: VERIFIED" :
                 s.strainer_inspected ? "FAULT INSPECTED - RESTORE HEADER" : "FIND AND INSPECT THE LO FAULT", kOrange);
            text(94, 213, s.lo_pressure_alarm && !s.alarm_acknowledged ? "X: ACKNOWLEDGE ALARM   O: CLOSE" :
                 s.mission_two_complete ? "MISSION PASSED!   O: CLOSE" :
                 "X: VERIFY RESTORATION   O: CLOSE", kTeal);
        }
    }
}

void GameApp::render() {
    // PSP SDL2 GU renderer requires an explicit clear before drawing.
    SDL_SetRenderDrawBlendMode(renderer_, SDL_BLENDMODE_NONE);
    SDL_SetRenderDrawColor(renderer_, kNavy.r, kNavy.g, kNavy.b, 255);
    SDL_RenderClear(renderer_);
    renderBackground();
    if (room_ == ControlRoom) renderControlRoom(); else renderEngineRoom();
    renderPlayer();
    renderHud();
    renderPanel();
    SDL_RenderPresent(renderer_);
}

} // namespace enginewatch
