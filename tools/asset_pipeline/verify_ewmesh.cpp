// Engine Watch G2 standalone PSP mesh stream verifier (native-host QA).
// .ewmesh format: EW3D + LE u32 version/count + N x Vertex3D ABGR/XYZ floats.
// This validates compatibility only; PSP runtime ingestion is a later gate.
#include "scene/SceneGeometry.hpp"

#include <cmath>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

using enginewatch::Vertex3D;

static_assert(sizeof(Vertex3D) == 16, "PSP colour XYZ vertices must be 16 bytes");

static uint32_t little32(const char* bytes) {
    return static_cast<uint32_t>(static_cast<unsigned char>(bytes[0])) |
           static_cast<uint32_t>(static_cast<unsigned char>(bytes[1])) << 8 |
           static_cast<uint32_t>(static_cast<unsigned char>(bytes[2])) << 16 |
           static_cast<uint32_t>(static_cast<unsigned char>(bytes[3])) << 24;
}

int main(int argc, char** argv) {
    if(argc < 2) {
        std::cerr << "Usage: verify_ewmesh file1.ewmesh [file2.ewmesh ...]\n";
        return 2;
    }
    const uint16_t endianProbe = 0x0102;
    if(reinterpret_cast<const unsigned char*>(&endianProbe)[0] != 0x02) {
        std::cerr << "Cannot reinterpret stream vertices on non-little-endian host\n";
        return 2;
    }
    for(int i=1; i<argc; ++i) {
        const std::string path(argv[i]);
        std::ifstream file(path.c_str(), std::ios::binary | std::ios::ate);
        if(!file) {
            std::cerr << path << ": file unavailable\n";
            return 1;
        }
        const std::streamoff length=file.tellg();
        if(length < 12 || length > 16*1024*1024) {
            std::cerr << path << ": invalid file size\n";
            return 1;
        }
        file.seekg(0,std::ios::beg);
        char header[12];
        if(!file.read(header,12)) {
            std::cerr << path << ": invalid header\n";
            return 1;
        }
        if(std::memcmp(header,"EW3D",4) != 0 || little32(header+4) != 1) {
            std::cerr << path << ": wrong format magic/version\n";
            return 1;
        }
        const uint32_t count=little32(header+8);
        if(count == 0 || count % 3 != 0 ||
           count > 1800u*3u ||
           static_cast<uint64_t>(length) != 12ull + 16ull*count) {
            std::cerr << path << ": invalid PSP geometry payload size\n";
            return 1;
        }
        std::vector<Vertex3D> mesh(count);
        if(!file.read(reinterpret_cast<char*>(mesh.data()),count*sizeof(Vertex3D))) {
            std::cerr << path << ": incomplete vertex stream\n";
            return 1;
        }
        for(std::size_t j=0;j<mesh.size();++j) {
            const Vertex3D& v=mesh[j];
            if((v.color >> 24) != 255u ||
               !std::isfinite(v.x) || !std::isfinite(v.y) ||
               !std::isfinite(v.z)) {
                std::cerr << path << ": corrupt ABGR/XYZ vertex " << j << "\n";
                return 1;
            }
            // Individual small parts should have sub-2 m local geometry,
            // unlike misplaced deck/wall surfaces from prior game builds.
            if(std::fabs(v.x) > 2.0f ||
               std::fabs(v.y) > 2.0f ||
               std::fabs(v.z) > 2.0f) {
                std::cerr << path << ": unrealistic local geometry extent\n";
                return 1;
            }
        }
        std::cout << "EW3D PASS: " << path << " ("
                  << count << " vertices / " << count/3 << " triangles)\n";
    }
    return 0;
}
