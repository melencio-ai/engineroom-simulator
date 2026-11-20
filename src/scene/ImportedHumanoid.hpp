#pragma once

#include "scene/SceneGeometry.hpp"
#include <cstddef>
#include <vector>

namespace enginewatch {

struct CharacterPose;

// Pre-baked CC0 source rig frames, converted OFFLINE in Blender. The PSP GE
// sees ordinary Vertex3D triangles: no runtime glTF parser or 80-bone skinning.
class ImportedHumanoid {
public:
    static const unsigned MaxVertices=1800;
    static bool load(const char* filename);
    static void clear();
    static bool available();
    static std::size_t storedBytes();
    static unsigned triangleCount();
    static bool sample(const Vec3& feet,float yaw,const CharacterPose& pose,
                       std::vector<Vertex3D>& out);
};

} // namespace enginewatch
