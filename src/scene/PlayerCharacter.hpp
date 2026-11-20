#pragma once

#include "scene/SceneGeometry.hpp"

#include <cstddef>
#include <vector>

namespace enginewatch {

// Low-poly marine-engineer rig; coordinates metres. This intentionally stays
// independent of PSP/SDL rendering and the G1-G2b machinery catalog.
enum CharacterAction {
    CharacterIdle=0,
    CharacterPressControl,
    CharacterInspectInstrument
};

struct CharacterPose {
    float walk_phase=0.0f;      // radians, driven by actual travel
    float motion=0.0f;          // 0..1, filtered walking/climbing intensity
    float clock_seconds=0.0f;
    float action_progress=0.0f; // 0..1 for a transient reach / inspection
    bool climbing=false;
    CharacterAction action=CharacterIdle;
    Vec3 target_world{0,0,0};
    bool has_target=false; // physical socket toward which right glove reaches
};

class CharacterAnimator {
public:
    void reset();
    void advance(float dt,float travelled_metres,bool climbing);
    void trigger(CharacterAction action,const Vec3* target=nullptr);

    const CharacterPose& pose() const { return pose_; }
private:
    CharacterPose pose_{};
    float action_elapsed_=0.0f;
};

// Reusable piecewise joint rig: torso, limbs, gloves, shoes, helmet and PPE.
// No heavy skinning matrices, textures, external allocations or GPU features
// are required. The body and hands are regenerated in a small transient
// vertex list; neither is appended to World3D's near-full static PSP mesh.
class PlayerCharacter {
public:
    static const float EyeHeight;
    static const float BodyHeight;
    static const std::size_t MaxRuntimeVertices=3000;

    // Optional imported CC0 Worker body; if the external sidecar is absent,
    // the proven G3.6 procedural engineer remains available on PSP/desktop.
    static bool loadImportedBody(const char* filename);
    static void clearImportedBody();
    static bool hasImportedBody();

    static void buildBody(const Vec3& feet,float yaw,const CharacterPose& pose,
                          std::vector<Vertex3D>& vertices);
    static void buildFirstPersonHands(const Vec3& eye,float yaw,float pitch,
                                     const CharacterPose& pose,
                                     std::vector<Vertex3D>& vertices);
};

} // namespace enginewatch
