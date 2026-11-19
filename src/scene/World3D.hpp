#pragma once

#include "scene/MarineParts.hpp"

#include <cstdint>
#include <vector>

namespace enginewatch {

// World units: metres. X longitudinal (forward to aft); Z transverse;
// Y vertical. PSP meshes use baked ABGR8888 colour vertices.
struct Footprint {
    float x0, x1, z0, z1;
    unsigned deck_mask;
};

enum DeckId {
    TankTopDeck = 0,
    PumpFlatDeck = 1,
    MainEngineDeck = 2,
    AuxiliaryDeck = 3,
    EcrDeck = 4,
    DeckCount = 5
};

enum StationId {
    NoStation = 0,
    EcrConsoleStation,
    LoPumpStation,
    StrainerStation
};

// An inclined stair repeats between adjacent floors, with landing loops around
// the open stairwell so each flight has a real top and bottom entry.
struct Stair3D {
    float top_x, top_z;
    float bottom_x, bottom_z;
    float width;
    bool alternate_escape;
};

class World3D {
public:
    World3D();

    const std::vector<Vertex3D>& vertices() const { return vertices_; }
    const std::vector<MarinePartPlacement>& parts() const { return parts_; }
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

private:
    std::vector<Vertex3D> vertices_;
    std::vector<MarinePartPlacement> parts_;
    MarinePartLibrary parts_library_;
    std::vector<Footprint> obstacles_;
    Stair3D stairs_[2];

    void triangle(const Vec3& a, const Vec3& b, const Vec3& c, uint32_t color);
    void quad(const Vec3& a, const Vec3& b, const Vec3& c, const Vec3& d, uint32_t color);
    void box(float cx,float cy,float cz,float hx,float hy,float hz,uint32_t color);
    void cylinderX(float x0,float x1,float y,float z,float radius,int segments,uint32_t color);
    void cylinderY(float x,float y0,float y1,float z,float radius,int segments,uint32_t color);
    void pipeZ(float z0,float z1,float x,float y,float radius,uint32_t color);
    void obstacle(float x0,float x1,float z0,float z1,unsigned deck_mask);
    void deckSlab(float x0,float x1,float z0,float z1,float height,uint32_t color);
    void railing(float x0,float x1,float z,float height,uint32_t color);
    void railingZ(float z0,float z1,float x,float height,uint32_t color);
    void stairGeometry(const Stair3D& stair);
    void part(MarinePartId type,MarineMaterialId finish,const PartTransform& position);
    void checkerTread(float x,float y,float z,uint32_t color);
    void plateSeams(float x0,float x1,float z0,float z1,float y);
    void lineBar(const Vec3& a,const Vec3& b,float half_width,uint32_t color);
    bool stairHeight(const Stair3D& stair,float x,float z,float feet_y,
                     float& candidate_y,float radius) const;
    void build();
};

} // namespace enginewatch
