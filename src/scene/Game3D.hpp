#pragma once

#include "core/EngineSimulation.hpp"
#include "scene/World3D.hpp"

namespace enginewatch {

struct Input3D {
    // Continuous inputs in [-1, 1]. The platform converts hardware controls.
    float forward = 0.0f;
    float strafe = 0.0f;
    float turn = 0.0f;
    // Single-frame button press edges (no repeating for action buttons).
    bool interact = false;
    bool back = false;
    bool help = false;
    bool pause = false;
    bool up = false;
    bool down = false;
};

struct Hud3D {
    char title[64];
    char status[96];
    char objective[96];
    char prompt[96];
    char panel_title[64];
    char panel_rows[10][96];
    int panel_count = 0;
    int selected_row = -1;
    bool panel_visible = false;
    bool paused = false;
    bool alarm = false;
};

class Game3D {
public:
    explicit Game3D(const World3D& world);

    void update(float dt_seconds, const Input3D& in);
    Hud3D hud() const;
    void reset();

    const Vec3& camera() const { return camera_; }
    float yaw() const { return yaw_; }
    bool isPaused() const { return paused_; }
    const EngineSimulation& simulation() const { return sim_; }

private:
    enum Panel { Closed, Console, Pumps, Strainer, Help };

    const World3D& world_;
    EngineSimulation sim_;
    Vec3 camera_{-7.35f, 1.65f, 0.0f};
    float yaw_ = 3.14159265358979323846f;
    Panel panel_ = Closed;
    int selected_ = 0;
    bool paused_ = false;

    void use();
    void move(float dt, float forward, float strafe, float turn);
    void operate();
};

} // namespace enginewatch
