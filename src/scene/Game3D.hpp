#pragma once

#include "core/EngineSimulation.hpp"
#include "scene/World3D.hpp"
#include "scene/PlayerCharacter.hpp"

namespace enginewatch {

struct Input3D {
    // Continuous movement/looking directions in [-1,1].
    float forward=0.0f;
    float strafe=0.0f;
    float turn=0.0f;
    float look=0.0f;
    // Edge-triggered actions: do not repeat while held.
    bool interact=false,back=false,help=false,pause=false,up=false,down=false;
    bool toggle_view=false; // PSP SELECT; desktop V; gamepad Back
};

// Camera is resolved by the game so both renderers share exactly the same
// first/third-person projection and collision-aware chase-camera fallback.
struct CameraView3D {
    Vec3 eye;
    Vec3 target;
    bool show_body=false;
    float fov_degrees=68.0f;
    bool constrained=false; // camera is shortened by physical geometry
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
    bool isThirdPerson() const { return third_person_; }
    const CharacterPose& characterPose() const { return character_animator_.pose(); }
    CameraView3D renderCamera() const;
    void buildCharacterGeometry(const CameraView3D& view,
                                std::vector<Vertex3D>& out) const;

private:
    enum Panel { Closed, Console, Pumps, Strainer, Help };
    const World3D& world_;
    EngineSimulation sim_;
    Vec3 camera_{-7.35f,14.45f,0.0f};
    float yaw_=0.0f;
    float pitch_=-0.16f;
    int deck_=EcrDeck;
    Panel panel_=Closed;
    int selected_=0;
    bool paused_=false;

    // Updated directly from physical slope collision; no teleport / ladder UI.
    bool climbing_=false;
    bool third_person_=false;
    float travel_this_frame_=0.0f;
    CharacterAnimator character_animator_;
    Vec3 chase_eye_{};
    Vec3 chase_target_{};
    float chase_distance_=0.0f;
    bool chase_initialized_=false;
    bool chase_constrained_=false;

    void use();
    void operate();
    void move(float dt,float forward,float strafe,float turn,float look);
    void updateChaseCamera(float dt);
};

} // namespace enginewatch
