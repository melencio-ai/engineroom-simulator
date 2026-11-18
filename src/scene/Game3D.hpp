#pragma once

#include "core/EngineSimulation.hpp"
#include "scene/World3D.hpp"

namespace enginewatch {

struct Input3D {
    // Continuous movement/looking directions in [-1,1].
    float forward=0.0f;
    float strafe=0.0f;
    float turn=0.0f;
    float look=0.0f;
    // Edge-triggered actions: do not repeat while held.
    bool interact=false,back=false,help=false,pause=false,up=false,down=false;
};

struct Hud3D {
    char title[64];
    char status[96];
    char objective[96];
    char prompt[96];
    char panel_title[64];
    char panel_rows[10][96];
    int panel_count=0;
    int selected_row=-1;
    bool panel_visible=false;
    bool paused=false;
    bool alarm=false;
    bool climbing=false;
};

class Game3D {
public:
    explicit Game3D(const World3D& world);
    void update(float dt_seconds,const Input3D& in);
    Hud3D hud() const;
    void reset();

    const Vec3& camera() const { return camera_; }
    float yaw() const { return yaw_; }
    float pitch() const { return pitch_; }
    int currentDeck() const { return deck_; }
    bool isClimbing() const { return climbing_; }
    bool isPaused() const { return paused_; }
    const EngineSimulation& simulation() const { return sim_; }

private:
    enum Panel { Closed, Console, Pumps, Strainer, Help, Ladder };
    const World3D& world_;
    EngineSimulation sim_;
    Vec3 camera_{-7.35f,14.45f,0.0f};
    float yaw_=0.0f;
    float pitch_=-0.16f;
    int deck_=EcrDeck;
    Panel panel_=Closed;
    int selected_=0;
    int ladder_index_=-1;
    bool paused_=false;

    // Smooth, camera-position-preserving vertical ladder traversal.
    bool climbing_=false;
    int climb_target_deck_=EcrDeck;
    float climb_elapsed_=0.0f;
    Vec3 climb_start_{};
    Vec3 climb_end_{};

    void use();
    void operate();
    void move(float dt,float forward,float strafe,float turn,float look);
    void beginClimb(int destination);
    void advanceClimb(float dt);
};

} // namespace enginewatch
