#pragma once

#include <cstdint>
#include <vector>

namespace enginewatch {

// World units: metres. X longitudinal (forward to aft); Z transverse;
// Y vertical. Colours are ABGR8888 for the PSP Graphics Utility.
struct Vec3 {
    float x;
    float y;
    float z;
};
struct Vertex3D {
    uint32_t color;
    float x;
    float y;
    float z;
};
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
    StrainerStation,
    MainLadderStation,
    EscapeLadderStation
};

struct Ladder3D {
    float landing_x, landing_z; // Where the player interacts safely on a deck
    float shaft_x, shaft_z;     // The visible, deck-cut-out ladder opening
    bool alternate_escape;
};

class World3D {
public:
    World3D();

    const std::vector<Vertex3D>& vertices() const { return vertices_; }
    static float floorHeight(int deck);
    static const char* deckName(int deck);
    static int cameraDeck(float eye_height);

    // Deck-scoped collision: walkable platforms, machinery and shaft cut-outs.
    bool canStand(float x, float z, int deck, float radius=0.28f) const;
    bool isInEcr(float x, int deck) const { return deck==EcrDeck && x < -3.4f; }

    // Interactions are only reachable from their correct level.
    StationId lookAtStation(const Vec3& camera, float yaw) const;
    const Ladder3D& ladder(int index) const { return ladders_[index==1 ? 1 : 0]; }
    int ladderCount() const { return 2; }

private:
    std::vector<Vertex3D> vertices_;
    std::vector<Footprint> obstacles_;
    Ladder3D ladders_[2];

    void triangle(const Vec3& a, const Vec3& b, const Vec3& c, uint32_t color);
    void quad(const Vec3& a, const Vec3& b, const Vec3& c, const Vec3& d, uint32_t color);
    void box(float cx,float cy,float cz,float hx,float hy,float hz,uint32_t color);
    void cylinderX(float x0,float x1,float y,float z,float radius,int segments,uint32_t color);
    void cylinderY(float x,float y0,float y1,float z,float radius,int segments,uint32_t color);
    void pipeZ(float z0,float z1,float x,float y,float radius,uint32_t color);
    void obstacle(float x0,float x1,float z0,float z1,unsigned deck_mask);
    void deckSlab(float x0,float x1,float z0,float z1,float height,uint32_t color);
    void railing(float x0,float x1,float z,float height,uint32_t color);
    void ladderGeometry(const Ladder3D& ladder);
    void build();
};

} // namespace enginewatch
