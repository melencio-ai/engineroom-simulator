#pragma once

#include <vector>

namespace enginewatch {

// One physical deck plan is used by both the rendered deck builder and the
// first-person floor/collision solver. Units are world metres; Y elevations
// are resolved separately from the deck identifier.
enum DeckId {
    TankTopDeck=0,
    PumpFlatDeck=1,
    MainEngineDeck=2,
    AuxiliaryDeck=3,
    EcrDeck=4,
    DeckCount=5
};

// Original inclined ladder geometry retained, but now defined only once.
struct Stair3D {
    float top_x,top_z;
    float bottom_x,bottom_z;
    float width;
    bool alternate_escape;
};

enum DeckSurface {
    NoDeckSurface=0,
    SolidSteelPlate,
    OpenSteelGrate,
    InsetServiceCover
};

enum OpeningPurpose {
    EngineMachineryWell=0,
    ForwardStairwell,
    AftEscapeStairwell
};

struct DeckCell {
    int level;
    DeckSurface surface;
    float x0,x1,z0,z1;
};

struct DeckOpening {
    int level;
    OpeningPurpose purpose;
    float x0,x1,z0,z1;
};

// Band groups drive underside bearer frames and opening-edge guard rails.
// The four visible grating bays are derived from the same band extents.
struct DeckGratingBand {
    int level;
    float x0,x1,z0,z1;
    int bay_count;
};

class DeckPlan {
public:
    DeckPlan();

    const std::vector<DeckCell>& cells() const { return cells_; }
    const std::vector<DeckOpening>& openings() const { return openings_; }
    const std::vector<DeckGratingBand>& gratingBands() const { return grating_bands_; }
    const Stair3D& stair(int index) const { return stairs_[index==1?1:0]; }

    // Does the *rendered* catalog actually contain floor support at the point?
    // An open steel grate counts as one walkable face although its slots are
    // deliberately open. This is an intentional gameplay collision proxy.
    DeckSurface surfaceAt(float x,float z,int level) const;

    // Inclusive union-of-cells support for a player's circular footprint.
    // Sampling the boundary AND midpoint avoids collision gaps at tile seams.
    bool supportsFootprint(float x,float z,int level,float radius) const;

    // Prevent a flat floor snap inside the physical stair/ladder envelope,
    // including L0 where a solid tank top exists below the inclined stair.
    bool insideStairLane(float x,float z,float radius) const;

    // Sanity checks: valid rectangles, no duplicate occupied cell, no cell
    // over an open machinery well or upper-stair shaft, covered grating bands.
    bool validate() const;

private:
    std::vector<DeckCell> cells_;
    std::vector<DeckOpening> openings_;
    std::vector<DeckGratingBand> grating_bands_;
    Stair3D stairs_[2];

    void plate(int level,float x0,float x1,float z0,float z1);
    void cell(int level,DeckSurface kind,float x0,float x1,float z0,float z1);
    void opening(int level,OpeningPurpose kind,float x0,float x1,float z0,float z1);
    void gratingBand(int level,float x0,float x1,float z0,float z1,int bays);
};

} // namespace enginewatch
