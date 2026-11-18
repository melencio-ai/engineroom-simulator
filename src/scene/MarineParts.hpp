#pragma once

#include "scene/World3D.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace enginewatch {

// FINISHES, not structural steel grades. PSP uses these as vertex-colour
// approximations; physical grades/pressure ratings are external metadata.
enum MarineMaterialId {
    GreenEpoxy = 0,
    GreenGrating,
    OffWhiteCoatedSteel,
    GalvanizedSteel,
    StructuralSteelGrey,
    MachineryBlue,
    PipeInsulation,
    CastIron,
    StainlessSteel,
    BronzeInstrument,
    SafetyYellow,
    LubricatingOilOrange,
    CoolingWaterBlue,
    FireSystemRed,
    RubberGasket,
    WorkLightWhite,
    MarineMaterialCount
};

struct MarineFinish {
    const char* name;
    uint32_t abgr;
    float roughness; // descriptive only: no PBR pipeline on PSP
    float metallic;  // descriptive only: no PBR pipeline on PSP
};

// Each type has ONE normalized local-space mesh, generated once and reused
// as many placed/scaled/rotated copies as a deck requires.
enum MarinePartId {
    SteelPlate = 0,
    CheckerPlate,
    OpenSteelGrating,
    IBeam,
    ChannelSection,
    AngleSection,
    BulbFlat,
    FlatBar,
    GussetBracket,
    StairTread,
    HandrailPost,
    HandrailSpan,
    PipeSpool,
    PipeElbow90,
    PipeTee,
    ConcentricReducer,
    WeldNeckFlange,
    BlindFlange,
    PipeGasket,
    HexBolt,
    HexNut,
    FlatWasher,
    GateValve,
    GlobeValve,
    ButterflyValve,
    YStrainer,
    PressureGauge,
    PipeClamp,
    PipeSupport,
    CableTray,
    AccessCover,
    WorkLightFixture,
    Ubolt,
    MarinePartCount
};

struct PartTransform {
    Vec3 position;
    Vec3 scale;
    float yaw_rad;   // Y-axis rotation
    float pitch_rad; // X-axis rotation
    float roll_rad;  // Z-axis rotation
    PartTransform(float x,float y,float z,
                  float sx=1.0f,float sy=1.0f,float sz=1.0f,
                  float yaw=0.0f,float pitch=0.0f,float roll=0.0f)
        :position{x,y,z},scale{sx,sy,sz},
         yaw_rad(yaw),pitch_rad(pitch),roll_rad(roll) {}
};

class MarinePartLibrary {
public:
    MarinePartLibrary();
    const MarineFinish& finish(MarineMaterialId id) const;
    const char* partName(MarinePartId id) const;
    std::size_t templateVertexCount(MarinePartId id) const;
    std::size_t templateCount() const { return static_cast<std::size_t>(MarinePartCount); }

    // PSP-compatible baked placements. No runtime GPU instancing is assumed:
    // templates are reused in C++, transformed into a single static mesh,
    // then submitted in PSP-safe bounded draw batches.
    bool append(std::vector<Vertex3D>& destination,MarinePartId part,
                MarineMaterialId material,const PartTransform& transform) const;

private:
    std::vector<Vertex3D> meshes_[MarinePartCount];
    void build();
};

} // namespace enginewatch
