#pragma once

#include "scene/MarineParts.hpp"
#include "scene/DeckLayout.hpp"

#include <cstdint>
#include <vector>

namespace enginewatch {

// World units: metres. X longitudinal (forward to aft); Z transverse;
// Y vertical. PSP meshes use baked ABGR8888 colour vertices.
struct Footprint {
    float x0, x1, z0, z1;
    unsigned deck_mask;
};

// Authoring-time 3D camera occluders match individual, real bulkhead,
// column, stair tread and equipment volumes. They never affect player
// locomotion, the static PSP draw mesh or the G3 deck collision model.
struct CameraOccluder {
    float x0,x1,y0,y1,z0,z1;
};

enum StationId {
    NoStation = 0,
    EcrConsoleStation,
    LoPumpStation,
    StrainerStation
};

// Stable engineering control identifiers. Every socket is authored next
// to its visible 3D equipment; positions are in vessel world metres.
// Six LO menu commands are LOCAL PANEL BUTTONS, not physical handwheels.
enum InteractionSocketId {
    NoInteractionSocket=0,
    LoMainStartButton, LoStandbyStartButton,
    LoMainSuctionButton, LoMainDischargeButton,
    LoStandbySuctionButton, LoStandbyDischargeButton,
    LoMainPressureGauge, LoStandbyPressureGauge,
    EcrVerifyButton, StrainerInspectionHandle,
    InteractionSocketCount
};
enum InteractionSocketKind {
    PanelPushbutton=0, GaugeInspection, EquipmentHandle
};
struct InteractionSocket {
    InteractionSocketId id;
    StationId station;
    InteractionSocketKind kind;
    Vec3 point;        // actual visible contact/inspection location
    Vec3 outward;      // surface normal pointing toward the operator
    float max_eye_distance;
};

class World3D {
public:
    World3D();

    const std::vector<Vertex3D>& vertices() const { return vertices_; }
    const std::vector<MarinePartPlacement>& parts() const { return parts_; }
    const DeckPlan& deckPlan() const { return deck_plan_; }
    const MarinePartLibrary& partCatalog() const { return parts_library_; }
    static float floorHeight(int deck);
    static const char* deckName(int deck);
    static int cameraDeck(float eye_height);

    // Deck-scoped collision: walkable platforms, machinery and stair openings.
    bool canStand(float x, float z, int deck, float radius=0.28f) const;
    // Resolve the continuous walkable height of a flat or sloping stair surface.
    // Call per horizontal movement step with the previous character foot height.
    bool walkSurface(float x, float z, float feet_y, float& next_feet_y,
                     float radius=0.25f) const;
    // Reject positions where a descending stair/stringer would intersect the
    // first-person camera/head. Kept separate from floor support resolution.
    bool hasHeadroom(float x, float z, float feet_y, float height=1.65f) const;
    bool onStair(float x, float z, float feet_y) const;
    bool isInEcr(float x, int deck) const { return deck==EcrDeck && x < -3.4f; }

    // Interactions are only reachable from their correct level.
    StationId lookAtStation(const Vec3& camera, float yaw) const;
    const Stair3D& stair(int index) const { return stairs_[index==1 ? 1 : 0]; }
    int stairCount() const { return 2; }

    // Returns the first unobstructed fraction of the segment [from,to].
    // 1.0 = fully visible, <1 = blocked by real ship structure. Inflated
    // AABBs approximate a small camera sphere; no per-triangle raycasts.
    float cameraRayFraction(const Vec3& from,const Vec3& to,
                            float radius=0.10f) const;
    std::size_t cameraOccluderCount() const { return camera_occluders_.size(); }

    const std::vector<InteractionSocket>& interactionSockets() const {
        return interaction_sockets_;
    }
    const InteractionSocket* interactionSocket(InteractionSocketId id) const;
    static InteractionSocketId pumpMenuSocket(int selected);
    // Mechanical reach proxy from the engineer's EYE (not just station
    // distance); an accurate wrist target is then solved by the character.
    bool canReachSocket(InteractionSocketId id,const Vec3& eye,
                        float facing_yaw) const;

private:
    std::vector<Vertex3D> vertices_;
    std::vector<MarinePartPlacement> parts_;
    MarinePartLibrary parts_library_;
    DeckPlan deck_plan_;
    std::vector<Footprint> obstacles_;
    std::vector<CameraOccluder> camera_occluders_;
    std::vector<InteractionSocket> interaction_sockets_;
    Stair3D stairs_[2];

    void triangle(const Vec3& a, const Vec3& b, const Vec3& c, uint32_t color);
    void quad(const Vec3& a, const Vec3& b, const Vec3& c, const Vec3& d, uint32_t color);
    void box(float cx,float cy,float cz,float hx,float hy,float hz,uint32_t color);
    // Split only exposed surfaces of a large slab/bulkhead into PSP-safe
    // screen-space tiles, without any redundant internal shared faces.
    void panelizedBox(float cx,float cy,float cz,float hx,float hy,float hz,
                      uint32_t color,float max_span=2.0f);
    void cylinderX(float x0,float x1,float y,float z,float radius,int segments,uint32_t color);
    void cylinderY(float x,float y0,float y1,float z,float radius,int segments,uint32_t color);
    void pipeZ(float z0,float z1,float x,float y,float radius,uint32_t color);
    void obstacle(float x0,float x1,float z0,float z1,unsigned deck_mask);
    void cameraBlocker(float cx,float cy,float cz,float hx,float hy,float hz);
    void addSocket(InteractionSocketId id,StationId station,
                   InteractionSocketKind kind,const Vec3& point,
                   const Vec3& outward,float reach);
    void occludingBox(float cx,float cy,float cz,float hx,float hy,float hz,
                      uint32_t color);
    void deckSlab(float x0,float x1,float z0,float z1,float height,uint32_t color);
    void railing(float x0,float x1,float z,float height,uint32_t color);
    void railingZ(float z0,float z1,float x,float height,uint32_t color);
    void stairGeometry(const Stair3D& stair);
    void part(MarinePartId type,MarineMaterialId finish,const PartTransform& position);
    void checkerTread(float x,float y,float z,uint32_t color);
    void lineBar(const Vec3& a,const Vec3& b,float half_width,uint32_t color);
    bool stairHeight(const Stair3D& stair,float x,float z,float feet_y,
                     float& candidate_y,float radius) const;
    void build();
};

} // namespace enginewatch
